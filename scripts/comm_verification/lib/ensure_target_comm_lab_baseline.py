#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import subprocess
import sys
import time

from run_target_can_matrix_probe import (
    ProbeFailure,
    apply_service_override,
    remove_service_override,
    service_environment,
    service_process_cmdlines,
    service_override_exists,
    ssh_command,
    shq,
    ssh_capture,
    systemctl_show,
    wait_service_active,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Ensure the governed target COMM lab baseline is ready.")
    parser.add_argument("--json-out", type=pathlib.Path, default=None, help="Optional JSON summary output path")
    return parser.parse_args()


def bool_env(name: str, default: bool = False) -> bool:
    value = os.getenv(name)
    if value is None:
        return default
    return value not in ("", "0", "false", "False", "no", "NO")


class TargetBaselineManager:
    def __init__(self) -> None:
        self.obc_target = os.environ["OBC_SSH_TARGET"]
        self.subsystem_target = os.environ["SUBSYSTEM_SIM_SSH_TARGET"]
        self.require_uhf_service = True
        self.ignore_gps_state_missing = bool_env("TARGET_BASELINE_IGNORE_GPS_STATE_MISSING", False)
        self.force_obc_comm_restart = bool_env("TARGET_BASELINE_FORCE_OBC_COMM_RESTART", False)
        self.journal_lines = int(os.getenv("TARGET_BASELINE_JOURNAL_LINES", "240"))
        self.restart_timeout = int(os.getenv("TARGET_BASELINE_RESTART_TIMEOUT_SEC", "120"))
        self.repair_settle_sec = float(os.getenv("TARGET_BASELINE_REPAIR_SETTLE_SEC", "5"))

        self.obc_can_service = os.getenv("OBC_CAN_SERVICE_NAME", "obc-lab-can.service")
        self.subsystem_eps_adcs_can_service = os.getenv(
            "SUBSYSTEM_EPS_ADCS_CAN_SERVICE_NAME", "subsystem-eps-adcs-lab-can.service"
        )
        self.subsystem_comm_can_service = os.getenv(
            "SUBSYSTEM_COMM_CAN_SERVICE_NAME", "subsystem-comm-lab-can.service"
        )
        self.obc_service = os.getenv("OBC_COMM_CSP_SERVICE_NAME", "obc-comm-csp-stack.service")
        self.eps_service = os.getenv("EPS_SERVICE_NAME", "subsystem-eps-csp.service")
        self.adcs_service = os.getenv("ADCS_SERVICE_NAME", "subsystem-adcs-csp.service")
        self.sband_service = os.getenv("SBAND_COMM_SERVICE_NAME", "subsystem-sband-csp.service")
        self.uhf_service = os.getenv("UHF_COMM_SERVICE_NAME", "subsystem-uhf-csp.service")
        self.sband_stack_target = os.getenv("SBAND_STACK_TARGET_NAME", "subsystem-sband-csp-stack.target")
        self.uhf_stack_target = os.getenv("UHF_STACK_TARGET_NAME", "subsystem-uhf-csp-stack.target")
        self.obc_can_device = os.getenv("OBC_CSP_CAN_DEVICE", "can0")
        self.subsystem_eps_adcs_can_device = os.getenv("SUBSYSTEM_SIM_CSP_CAN_DEVICE", "can0")
        self.subsystem_comm_can_device = os.getenv("SUBSYSTEM_SIM_COMM_CAN_DEVICE", "can1")
        self.sband_tcp_port = int(os.getenv("SBAND_TCP_PORT", "18520"))
        self.canfd_dropin_name = os.getenv(
            "TARGET_BASELINE_CANFD_DROPIN_NAME", "57-csp-socketcan-canfd.conf"
        )
        self.expected_comm_canfd_env = {
            "COMM_CSP_SOCKETCAN_USE_CANFD": os.getenv(
                "TARGET_BASELINE_COMM_CSP_SOCKETCAN_USE_CANFD", "1"
            ),
            "COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST": os.getenv(
                "TARGET_BASELINE_COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST", "5,6"
            ),
            "COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST": os.getenv(
                "TARGET_BASELINE_COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST", "40"
            ),
        }
        self.beacon_sidecar_root = os.getenv("TARGET_BEACON_SIDECAR_ROOT", "/tmp/target-beacon-sidecar-baseline-v1")
        self.beacon_sidecar_label = os.getenv("TARGET_BEACON_SIDECAR_LABEL", "target-beacon-sidecar-baseline-v1")
        self.beacon_obc_dropin = os.getenv("TARGET_BEACON_OBC_DROPIN", "58-target-beacon-baseline.conf")
        self.beacon_uhf_dropin = os.getenv("TARGET_BEACON_UHF_DROPIN", "59-target-beacon-baseline.conf")

        self.expected_obc_env = {
            "TARGET_COMM_PROFILE": "sband",
            "COMM_CSP_NODE": "5",
            "COMMAND_AUTHORITY_PROFILE": "sband-primary",
            "OBC_GPS_SOURCE_MODE": "live-uart",
        }

        self.obc_dropins = [
            "50-diag-quiet-packet-egress.conf",
            "51-target-comm-profile-override.conf",
            "52-uhf-beacon-csp-node.conf",
            "53-obc-reliable-transfer-admission.conf",
            "55-obc-groundlink-diagnostics.conf",
            "56-obc-groundlink-timeouts.conf",
            "57-uhf-primary-live-packets.conf",
            "59-chapter5-ttc-gps-replay.conf",
            "60-rg1-timing-probe.conf",
            "99-disable-hw-watchdog.conf",
        ]
        self.eps_dropins = ["57-csp-socketcan-canfd.conf"]
        self.adcs_dropins = ["57-csp-socketcan-canfd.conf"]
        self.sband_dropins = ["58-sband-ingress-diagnostics.conf"]
        self.uhf_dropins = [
            "52-reliable-transfer-output.conf",
            "53-uhf-beacon-capture.conf",
            "54-uhf-ingress-diagnostics.conf",
        ]

        self.service_patterns = {
            (self.obc_target, self.obc_service): {
                "run_obc_comm_csp_stack.sh": re.compile(r"run_obc_comm_csp_stack\.sh"),
                "radio_mock_server": re.compile(r"(^|/)\S*radio_mock_server(?:\s|$)"),
                "OBC": re.compile(r"(^|/)\S*/OBC(?:\s|$)"),
            },
            (self.subsystem_target, self.eps_service): {
                "eps_simulator": re.compile(r"(^|/)\S*eps_simulator(?:\s|$)"),
            },
            (self.subsystem_target, self.adcs_service): {
                "adcs_simulator": re.compile(r"(^|/)\S*adcs_simulator(?:\s|$)"),
            },
            (self.subsystem_target, self.sband_service): {
                "sband_comm_csp_node": re.compile(r"(^|/)\S*sband_comm_csp_node(?:\s|$)"),
            },
            (self.subsystem_target, self.uhf_service): {
                "uhf_comm_csp_node": re.compile(r"(^|/)\S*uhf_comm_csp_node(?:\s|$)"),
            },
        }

    def service_state(self, target: str, unit: str) -> dict[str, object]:
        show = systemctl_show(target, unit, ("ActiveState", "SubState", "MainPID", "UnitFileState"))
        return {
            "activeState": show.get("ActiveState", ""),
            "subState": show.get("SubState", ""),
            "mainPid": show.get("MainPID", ""),
            "unitFileState": show.get("UnitFileState", ""),
        }

    def fetch_recent_journal_with_scope(self, target: str, unit: str, lines: int | None = None) -> tuple[str, bool]:
        props = systemctl_show(target, unit, ("InvocationID",))
        invocation_id = props.get("InvocationID", "").strip()
        if invocation_id:
            return (
                ssh_capture(
                    target,
                    f"journalctl _SYSTEMD_INVOCATION_ID={shq(invocation_id)} --no-pager || true",
                    check=False,
                ),
                True,
            )
        count = lines or self.journal_lines
        return (
            ssh_capture(
                target,
                f"journalctl -u {shq(unit)} -n {count} --no-pager || true",
                check=False,
            ),
            False,
        )

    def fetch_recent_journal(self, target: str, unit: str, lines: int | None = None) -> str:
        journal, _ = self.fetch_recent_journal_with_scope(target, unit, lines)
        return journal

    def can_state(self, target: str, device: str) -> dict[str, object]:
        text = ssh_capture(target, f"ip -details -statistics link show dev {shq(device)} || true", check=False)
        state_match = re.search(rf"{re.escape(device)}: .* state ([A-Z-]+)", text)
        restart_match = re.search(r"restart-ms (\d+)", text)
        bus_off_match = re.search(r"bus-off (\d+)", text)
        return {
            "present": bool(text.strip()),
            "state": state_match.group(1) if state_match else "",
            "restartMs": int(restart_match.group(1)) if restart_match else None,
            "busOffCount": int(bus_off_match.group(1)) if bus_off_match else None,
            "raw": text,
        }

    def sband_listener_ready(self) -> bool:
        text = ssh_capture(
            self.subsystem_target,
            f"ss -ltn '( sport = :{self.sband_tcp_port} )' | tail -n +2 || true",
            check=False,
        )
        return bool(text.strip())

    @staticmethod
    def latest_link_state(
        journal: str,
        stem: str,
        *,
        invocation_scoped: bool,
        availability_fragment: str | None = None,
    ) -> str | None:
        for line in reversed(journal.splitlines()):
            if not invocation_scoped and "OBC CCSDS S-band runtime started." in line:
                break
            if availability_fragment is not None:
                if availability_fragment in line:
                    if "available 1" in line:
                        return "UP"
                    if "available 0" in line:
                        return "DOWN"
                    return None
                continue
            if f"{stem}) GROUND_LINK_UP" in line:
                return "UP"
            if f"{stem}) GROUND_LINK_DOWN" in line:
                return "DOWN"
        return None

    @staticmethod
    def has_recent_fragment(journal: str, fragment: str, *, invocation_scoped: bool) -> bool:
        for line in reversed(journal.splitlines()):
            if fragment in line:
                return True
            if not invocation_scoped and "OBC CCSDS S-band runtime started." in line:
                break
        return False

    def dropin_exists(self, target: str, service: str, dropin_name: str) -> bool:
        return service_override_exists(target, service, dropin_name)

    def list_remote_processes(self, target: str) -> list[dict[str, object]]:
        script = (
            "python3 -c "
            + shq(
                "import json, subprocess\n"
                "out = subprocess.run(['ps','-ax','-o','pid=','-o','command='], check=False, capture_output=True, text=True).stdout\n"
                "rows=[]\n"
                "for line in out.splitlines():\n"
                "    stripped=line.strip()\n"
                "    if not stripped:\n"
                "        continue\n"
                "    parts=stripped.split(None, 1)\n"
                "    if len(parts)!=2:\n"
                "        continue\n"
                "    pid_text, command = parts\n"
                "    try:\n"
                "        pid=int(pid_text)\n"
                "    except ValueError:\n"
                "        continue\n"
                "    rows.append({'pid': pid, 'command': command})\n"
                "print(json.dumps(rows, sort_keys=True))"
            )
        )
        text = ssh_capture(target, script, check=False).strip()
        if not text:
            return []
        try:
            parsed = json.loads(text)
        except json.JSONDecodeError:
            return []
        return parsed if isinstance(parsed, list) else []

    def duplicate_processes(self) -> list[dict[str, object]]:
        duplicates: list[dict[str, object]] = []
        remote_process_cache = {
            self.obc_target: self.list_remote_processes(self.obc_target),
            self.subsystem_target: self.list_remote_processes(self.subsystem_target),
        }
        for (target, service), family_patterns in self.service_patterns.items():
            descendants = {
                int(entry["pid"])
                for entry in service_process_cmdlines(target, service)
                if str(entry.get("pid", "")).isdigit()
            }
            for family, pattern in family_patterns.items():
                matches = [
                    entry
                    for entry in remote_process_cache[target]
                    if pattern.search(str(entry.get("command", "")))
                ]
                extras = [entry for entry in matches if int(entry["pid"]) not in descendants]
                for entry in extras:
                    duplicates.append(
                        {
                            "target": target,
                            "service": service,
                            "family": family,
                            "pid": int(entry["pid"]),
                            "command": str(entry["command"]),
                        }
                    )
        return duplicates

    def kill_remote_duplicates(self, duplicates: list[dict[str, object]]) -> list[int]:
        pids_by_target: dict[str, set[int]] = {}
        for row in duplicates:
            pids_by_target.setdefault(str(row["target"]), set()).add(int(row["pid"]))
        killed: list[int] = []
        for target, pid_set in pids_by_target.items():
            pid_list = sorted(pid_set)
            if not pid_list:
                continue
            script = (
                "python3 -c "
                + shq(
                    "import os, signal, sys, time\n"
                    "pids=[int(x) for x in sys.argv[1:]]\n"
                    "for sig in (signal.SIGTERM, signal.SIGKILL):\n"
                    "    for pid in pids:\n"
                    "        try:\n"
                    "            os.kill(pid, sig)\n"
                    "        except ProcessLookupError:\n"
                    "            pass\n"
                    "        except PermissionError:\n"
                    "            pass\n"
                    "    time.sleep(1 if sig == signal.SIGTERM else 0)\n"
                )
                + " "
                + " ".join(shq(str(pid)) for pid in pid_list)
            )
            ssh_capture(target, script, check=False)
            killed.extend(pid_list)
        return killed

    def collect_state(self) -> dict[str, object]:
        obc_env = service_environment(self.obc_target, self.obc_service)
        sband_env = service_environment(self.subsystem_target, self.sband_service)
        uhf_env = service_environment(self.subsystem_target, self.uhf_service)
        obc_journal, obc_invocation_scoped = self.fetch_recent_journal_with_scope(self.obc_target, self.obc_service)
        subsystem_sband_journal = self.fetch_recent_journal(self.subsystem_target, self.sband_service)
        subsystem_uhf_journal = self.fetch_recent_journal(self.subsystem_target, self.uhf_service, 120)
        state = {
            "services": {
                "obcLabCan": self.service_state(self.obc_target, self.obc_can_service),
                "obcComm": self.service_state(self.obc_target, self.obc_service),
                "subsystemEpsAdcsCan": self.service_state(self.subsystem_target, self.subsystem_eps_adcs_can_service),
                "subsystemCommCan": self.service_state(self.subsystem_target, self.subsystem_comm_can_service),
                "subsystemEps": self.service_state(self.subsystem_target, self.eps_service),
                "subsystemAdcs": self.service_state(self.subsystem_target, self.adcs_service),
                "subsystemSband": self.service_state(self.subsystem_target, self.sband_service),
                "subsystemUhf": self.service_state(self.subsystem_target, self.uhf_service),
                "subsystemSbandTarget": self.service_state(self.subsystem_target, self.sband_stack_target),
                "subsystemUhfTarget": self.service_state(self.subsystem_target, self.uhf_stack_target),
            },
            "can": {
                "obcCan0": self.can_state(self.obc_target, self.obc_can_device),
                "subsystemCan0": self.can_state(self.subsystem_target, self.subsystem_eps_adcs_can_device),
                "subsystemCan1": self.can_state(self.subsystem_target, self.subsystem_comm_can_device),
            },
            "sbandListenerReady": self.sband_listener_ready(),
            "obcEnvironment": {key: obc_env.get(key, "") for key in self.expected_obc_env},
            "commCanFdProfile": {
                "obc": {key: obc_env.get(key, "") for key in self.expected_comm_canfd_env},
                "sband": {key: sband_env.get(key, "") for key in self.expected_comm_canfd_env},
                "uhf": {key: uhf_env.get(key, "") for key in self.expected_comm_canfd_env},
            },
            "journalMarkers": {
                "obcRuntimeStartedSeen": self.has_recent_fragment(
                    obc_journal, "OBC CCSDS S-band runtime started.", invocation_scoped=obc_invocation_scoped
                ),
                "obcGroundLinkConfiguredSeen": self.has_recent_fragment(
                    obc_journal, "Ground link via COMM CSP node: 5", invocation_scoped=obc_invocation_scoped
                ),
                "obcCspInitSeen": self.has_recent_fragment(
                    obc_journal, "CSP initialized for node 1", invocation_scoped=obc_invocation_scoped
                ),
                "obcSbandAvailabilityState": self.latest_link_state(
                    obc_journal,
                    "groundLinkDriver",
                    invocation_scoped=obc_invocation_scoped,
                    availability_fragment="COMM_LINK_AVAILABILITY_CHANGED : Comm link SBAND (0) available ",
                ),
                "obcSbandGroundLinkState": self.latest_link_state(
                    obc_journal, "groundLinkDriver", invocation_scoped=obc_invocation_scoped
                ),
                "obcUhfGroundLinkState": self.latest_link_state(
                    obc_journal, "uhfGroundLinkDriver", invocation_scoped=obc_invocation_scoped
                ),
                "sbandService": "listening" in subsystem_sband_journal.lower() or "tcp-listen-port" in subsystem_sband_journal,
                "uhfService": "serial-device" in subsystem_uhf_journal or "node 6" in subsystem_uhf_journal,
            },
            "overrides": {
                "obc": [name for name in self.obc_dropins if self.dropin_exists(self.obc_target, self.obc_service, name)],
                "eps": [name for name in self.eps_dropins if self.dropin_exists(self.subsystem_target, self.eps_service, name)],
                "adcs": [name for name in self.adcs_dropins if self.dropin_exists(self.subsystem_target, self.adcs_service, name)],
                "sband": [name for name in self.sband_dropins if self.dropin_exists(self.subsystem_target, self.sband_service, name)],
                "uhf": [name for name in self.uhf_dropins if self.dropin_exists(self.subsystem_target, self.uhf_service, name)],
            },
            "duplicates": self.duplicate_processes(),
        }
        return state

    def analyze(self, state: dict[str, object]) -> list[str]:
        issues: list[str] = []
        services = state["services"]
        required_service_keys = [
            "obcLabCan",
            "obcComm",
            "subsystemEpsAdcsCan",
            "subsystemCommCan",
            "subsystemEps",
            "subsystemAdcs",
            "subsystemSband",
        ]
        required_service_keys.extend(["subsystemUhf", "subsystemUhfTarget"])
        required_service_keys.append("subsystemSbandTarget")
        for key in required_service_keys:
            if services[key]["activeState"] != "active":
                issues.append(f"service-inactive:{key}")

        can = state["can"]
        for key in ("obcCan0", "subsystemCan0", "subsystemCan1"):
            row = can[key]
            if not row["present"]:
                issues.append(f"can-missing:{key}")
                continue
            if row["state"] in ("DOWN", "BUS-OFF"):
                issues.append(f"can-state:{key}:{row['state']}")
            if row["restartMs"] != 100:
                issues.append(f"can-restart-ms:{key}:{row['restartMs']}")

        env = state["obcEnvironment"]
        for key, expected in self.expected_obc_env.items():
            if env.get(key, "") != expected:
                issues.append(f"obc-env:{key}:{env.get(key, '')}->{expected}")

        for family, profile_env in state["commCanFdProfile"].items():
            for key, expected in self.expected_comm_canfd_env.items():
                if profile_env.get(key, "") != expected:
                    issues.append(
                        f"comm-canfd-profile:{family}:{key}:{profile_env.get(key, '')}->{expected}"
                    )

        journal = state["journalMarkers"]
        if not journal["obcRuntimeStartedSeen"]:
            issues.append("journal:obc-runtime-start-missing")
        if not journal["obcGroundLinkConfiguredSeen"]:
            issues.append("journal:obc-ground-link-config-missing")
        if not journal["obcCspInitSeen"]:
            issues.append("journal:obc-csp-init-missing")
        if journal["obcSbandAvailabilityState"] != "UP":
            issues.append(f"journal:obc-sband-availability-state:{journal['obcSbandAvailabilityState']}")
        if not state["sbandListenerReady"]:
            issues.append("listener:sband-tcp-missing")

        for family, names in state["overrides"].items():
            for name in names:
                issues.append(f"override-present:{family}:{name}")

        for row in state["duplicates"]:
            issues.append(f"duplicate:{row['target']}:{row['family']}:{row['pid']}")
        return issues

    def remove_overrides(self, repairs: list[str], state: dict[str, object]) -> None:
        for name in state["overrides"]["obc"]:
            remove_service_override(self.obc_target, self.obc_service, name)
            wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
            repairs.append(f"removed-obc-override:{name}")
        for name in state["overrides"]["eps"]:
            remove_service_override(self.subsystem_target, self.eps_service, name)
            wait_service_active(self.subsystem_target, self.eps_service, self.restart_timeout)
            repairs.append(f"removed-eps-override:{name}")
        for name in state["overrides"]["adcs"]:
            remove_service_override(self.subsystem_target, self.adcs_service, name)
            wait_service_active(self.subsystem_target, self.adcs_service, self.restart_timeout)
            repairs.append(f"removed-adcs-override:{name}")
        for name in state["overrides"]["sband"]:
            remove_service_override(self.subsystem_target, self.sband_service, name)
            wait_service_active(self.subsystem_target, self.sband_service, self.restart_timeout)
            repairs.append(f"removed-sband-override:{name}")
        for name in state["overrides"]["uhf"]:
            remove_service_override(self.subsystem_target, self.uhf_service, name)
            wait_service_active(self.subsystem_target, self.uhf_service, self.restart_timeout)
            repairs.append(f"removed-uhf-override:{name}")

    def ensure_comm_canfd_profile(self, repairs: list[str]) -> None:
        services = (
            (self.subsystem_target, self.sband_service, "sband"),
            (self.subsystem_target, self.uhf_service, "uhf"),
            (self.obc_target, self.obc_service, "obc"),
        )
        for target, service, family in services:
            current = service_environment(target, service)
            if all(current.get(key, "") == value for key, value in self.expected_comm_canfd_env.items()):
                continue
            apply_service_override(
                target,
                service,
                self.canfd_dropin_name,
                self.expected_comm_canfd_env,
            )
            wait_service_active(target, service, self.restart_timeout)
            repairs.append(f"applied-comm-canfd-profile:{family}:{self.canfd_dropin_name}")

    def ensure_beacon_sidecar(self, repairs: list[str]) -> dict[str, object]:
        """Create or verify the A-owned UHF Beacon capture sidecar."""
        working_directory = ssh_capture(
            self.subsystem_target,
            f"systemctl show {shq(self.uhf_service)} --property=WorkingDirectory --value",
        ).strip()
        if not working_directory:
            raise ProbeFailure(f"missing WorkingDirectory for {self.uhf_service}")
        bridge_binary = f"{working_directory}/build-fprime-automatic-native/bin/Linux/pty_pair_bridge"
        capture_helper = f"{working_directory}/scripts/manual_ops/lib/beacon_sidecar.py"
        remote_program = (
            "import json,os,pathlib,signal,subprocess,sys,time\n"
            "root=pathlib.Path(sys.argv[1]); bridge=sys.argv[2]; helper=sys.argv[3]; label=sys.argv[4]\n"
            "root.mkdir(parents=True,exist_ok=True)\n"
            "def matching_process(path,*needles):\n"
            "  try:\n"
            "    pid=int(path.read_text().strip()); os.kill(pid,0)\n"
            "    cmd=pathlib.Path('/proc')/str(pid)/'cmdline'\n"
            "    return all(needle.encode() in cmd.read_bytes() for needle in needles)\n"
            "  except (OSError,ValueError,FileNotFoundError): return False\n"
            "def terminate_matching_processes(*needles):\n"
            "  for candidate in pathlib.Path('/proc').iterdir():\n"
            "    if not candidate.name.isdigit(): continue\n"
            "    try:\n"
            "      cmd=(candidate/'cmdline').read_bytes()\n"
            "      if all(needle.encode() in cmd for needle in needles): os.kill(int(candidate.name),signal.SIGTERM)\n"
            "    except (OSError,ValueError): pass\n"
            "def bridge_metadata_ready():\n"
            "  try:\n"
            "    lines=dict(line.strip().split('=',1) for line in bridge_log.read_text(errors='replace').splitlines() if '=' in line)\n"
            "    return 'PTY_A' in lines and 'PTY_B' in lines and pathlib.Path(lines['PTY_A']).exists() and pathlib.Path(lines['PTY_B']).exists()\n"
            "  except OSError: return False\n"
            "bridge_pid=root/'bridge.pid'; capture_pid=root/'capture.pid'; bridge_log=root/'bridge.log'; capture_log=root/'capture.log'; capture=root/'uhf-beacon.bin'\n"
            "if not matching_process(bridge_pid,bridge,'--instance-label',label) or not bridge_metadata_ready():\n"
            "  if matching_process(capture_pid,helper,'capture','--output',str(capture)):\n"
            "    try: os.kill(int(capture_pid.read_text().strip()), signal.SIGTERM)\n"
            "    except (OSError, ValueError): pass\n"
            "  subprocess.run(['pkill','-f','--','--instance-label '+label],check=False,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)\n"
            "  bridge_log.unlink(missing_ok=True); capture_pid.unlink(missing_ok=True); capture.unlink(missing_ok=True)\n"
            "  with bridge_log.open('wb') as h: p=subprocess.Popen([bridge,'--instance-label',label],stdin=subprocess.DEVNULL,stdout=h,stderr=subprocess.STDOUT,start_new_session=True)\n"
            "  bridge_pid.write_text(str(p.pid)+'\\n')\n"
            "for _ in range(100):\n"
            "  lines=dict(line.strip().split('=',1) for line in bridge_log.read_text(errors='replace').splitlines() if '=' in line) if bridge_log.exists() else {}\n"
            "  if 'PTY_A' in lines and 'PTY_B' in lines: break\n"
            "  time.sleep(.1)\n"
            "else: raise RuntimeError('target Beacon bridge did not report PTY paths')\n"
            "if not pathlib.Path(lines['PTY_A']).exists() or not pathlib.Path(lines['PTY_B']).exists(): raise RuntimeError('target Beacon bridge PTY path is unavailable')\n"
            "if not capture.exists() or not matching_process(capture_pid,helper,'capture','--device',lines['PTY_B'],'--output',str(capture)):\n"
            "  terminate_matching_processes(helper,'capture','--output',str(capture))\n"
            "  with capture_log.open('ab') as h: p=subprocess.Popen(['python3',helper,'capture','--device',lines['PTY_B'],'--output',str(capture)],stdin=subprocess.DEVNULL,stdout=h,stderr=subprocess.STDOUT,start_new_session=True)\n"
            "  capture_pid.write_text(str(p.pid)+'\\n')\n"
            "print(json.dumps({'ready':matching_process(bridge_pid,bridge,'--instance-label',label) and matching_process(capture_pid,helper,'capture','--device',lines['PTY_B'],'--output',str(capture)),'sourceKind':'target-remote-sidecar','sourceBand':'uhf-backup','subsystemTarget':sys.argv[5],'capturePath':str(capture),'frameSize':108,'instanceLabel':label,'bridgePid':int(bridge_pid.read_text()),'capturePid':int(capture_pid.read_text()),'ptyDevice':lines['PTY_A'],'uhfService':sys.argv[6]}))\n"
        )
        command = "python3 -c " + shq(remote_program) + " " + " ".join(
            shq(value)
            for value in (
                self.beacon_sidecar_root,
                bridge_binary,
                capture_helper,
                self.beacon_sidecar_label,
                self.subsystem_target,
                self.uhf_service,
            )
        )
        try:
            sidecar = json.loads(ssh_capture(self.subsystem_target, command))
        except (ValueError, RuntimeError, ProbeFailure) as exc:
            raise ProbeFailure(f"target Beacon sidecar setup failed: {exc}") from exc
        if not sidecar.get("ready"):
            raise ProbeFailure("target Beacon sidecar is not ready")
        expected_uhf = {"SUBSYSTEM_SIM_COMM_BEACON_DEVICE": str(sidecar["ptyDevice"])}
        if any(service_environment(self.subsystem_target, self.uhf_service).get(key) != value for key, value in expected_uhf.items()):
            apply_service_override(self.subsystem_target, self.uhf_service, self.beacon_uhf_dropin, expected_uhf)
            wait_service_active(self.subsystem_target, self.uhf_service, self.restart_timeout)
            repairs.append(f"applied-target-beacon-sidecar:{self.uhf_service}")
        expected_obc = {"UHF_BEACON_CSP_NODE": "6"}
        if any(service_environment(self.obc_target, self.obc_service).get(key) != value for key, value in expected_obc.items()):
            apply_service_override(self.obc_target, self.obc_service, self.beacon_obc_dropin, expected_obc)
            wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
            repairs.append(f"applied-target-beacon-sidecar:{self.obc_service}")
        return sidecar

    def restart_service(self, target: str, unit: str) -> None:
        self.service_control(target, "restart", unit)
        wait_service_active(target, unit, self.restart_timeout)

    def start_service(self, target: str, unit: str) -> None:
        self.service_control(target, "start", unit)
        wait_service_active(target, unit, self.restart_timeout)

    def stop_service(self, target: str, unit: str) -> None:
        self.service_control(target, "stop", unit, check=False)

    def service_control(self, target: str, action: str, unit: str, *, check: bool = True) -> None:
        deadline = time.monotonic() + self.restart_timeout
        script = f"sudo systemctl {action} {shq(unit)}"
        last_error: str | None = None
        while True:
            result = subprocess.run(
                ssh_command(target, script),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )
            if result.returncode == 0:
                return
            last_error = (
                f"ssh command failed target={target} rc={result.returncode}\n"
                f"cmd={script}\nstdout={result.stdout}\nstderr={result.stderr}"
            )
            transient_ssh_failure = result.returncode == 255
            if transient_ssh_failure and time.monotonic() < deadline:
                time.sleep(2.0)
                continue
            if check:
                raise ProbeFailure(last_error)
            return

    def perform_transport_reset(self, repairs: list[str], state: dict[str, object]) -> None:
        self.stop_service(self.obc_target, self.obc_service)
        repairs.append(f"stopped:{self.obc_service}")

        subsystem_stop_units = [
            self.sband_stack_target,
            self.sband_service,
            self.uhf_stack_target,
            self.uhf_service,
            self.adcs_service,
            self.eps_service,
        ]
        for unit in subsystem_stop_units:
            self.stop_service(self.subsystem_target, unit)
            repairs.append(f"stopped:{unit}")

        self.restart_service(self.obc_target, self.obc_can_service)
        repairs.append(f"transport-reset-restarted:{self.obc_can_service}")
        self.restart_service(self.subsystem_target, self.subsystem_eps_adcs_can_service)
        repairs.append(f"transport-reset-restarted:{self.subsystem_eps_adcs_can_service}")
        self.restart_service(self.subsystem_target, self.subsystem_comm_can_service)
        repairs.append(f"transport-reset-restarted:{self.subsystem_comm_can_service}")

        subsystem_start_units = [
            self.eps_service,
            self.adcs_service,
            self.sband_service,
            self.sband_stack_target,
            self.uhf_service,
            self.uhf_stack_target,
        ]
        for unit in subsystem_start_units:
            self.start_service(self.subsystem_target, unit)
            repairs.append(f"transport-reset-started:{unit}")

        time.sleep(self.repair_settle_sec)
        self.start_service(self.obc_target, self.obc_service)
        repairs.append(f"transport-reset-started:{self.obc_service}")
        time.sleep(self.repair_settle_sec)

    def perform_repairs(self, state: dict[str, object], issues: list[str]) -> list[str]:
        repairs: list[str] = []
        if state["duplicates"]:
            killed = self.kill_remote_duplicates(state["duplicates"])
            if killed:
                repairs.append("killed-duplicate-pids:" + ",".join(str(pid) for pid in sorted(killed)))

        if any(state["overrides"].values()):
            self.remove_overrides(repairs, state)

        if any(issue.startswith("comm-canfd-profile:") for issue in issues):
            self.ensure_comm_canfd_profile(repairs)

        obc_sband_availability_broken = any(
            issue.startswith("journal:obc-sband-availability-state:") for issue in issues
        )
        obc_can_needs_restart = any(
            issue.startswith("can-") and "obcCan0" in issue for issue in issues
        ) or "service-inactive:obcLabCan" in issues or obc_sband_availability_broken
        subsystem_can_needs_restart = any(
            issue.startswith("can-") and ("subsystemCan0" in issue or "subsystemCan1" in issue) for issue in issues
        ) or "service-inactive:subsystemEpsAdcsCan" in issues or "service-inactive:subsystemCommCan" in issues

        if subsystem_can_needs_restart:
            self.restart_service(self.subsystem_target, self.subsystem_eps_adcs_can_service)
            repairs.append(f"restarted:{self.subsystem_eps_adcs_can_service}")
            self.restart_service(self.subsystem_target, self.subsystem_comm_can_service)
            repairs.append(f"restarted:{self.subsystem_comm_can_service}")

        if obc_can_needs_restart:
            self.restart_service(self.obc_target, self.obc_can_service)
            repairs.append(f"restarted:{self.obc_can_service}")

        subsystem_units = [self.eps_service, self.adcs_service, self.sband_service, self.sband_stack_target]
        subsystem_units.extend([self.uhf_service, self.uhf_stack_target])
        sband_listener_missing = any(issue.startswith("listener:sband-tcp-missing") for issue in issues)
        for unit in subsystem_units:
            if (
                unit == self.sband_service and sband_listener_missing
            ) or (
                unit == self.sband_stack_target and sband_listener_missing
            ):
                self.restart_service(self.subsystem_target, unit)
                repairs.append(f"restarted:{unit}")
            elif f"service-inactive:{self._service_state_key(unit)}" in issues or subsystem_can_needs_restart:
                self.start_service(self.subsystem_target, unit)
                repairs.append(f"started:{unit}")

        if (
            "service-inactive:obcComm" in issues
            or obc_can_needs_restart
            or any(issue.startswith("obc-env:") or issue.startswith("journal:") for issue in issues)
        ):
            self.restart_service(self.obc_target, self.obc_service)
            repairs.append(f"restarted:{self.obc_service}")

        time.sleep(self.repair_settle_sec)
        return repairs

    def _service_state_key(self, unit: str) -> str:
        mapping = {
            self.obc_can_service: "obcLabCan",
            self.obc_service: "obcComm",
            self.subsystem_eps_adcs_can_service: "subsystemEpsAdcsCan",
            self.subsystem_comm_can_service: "subsystemCommCan",
            self.eps_service: "subsystemEps",
            self.adcs_service: "subsystemAdcs",
            self.sband_service: "subsystemSband",
            self.uhf_service: "subsystemUhf",
            self.sband_stack_target: "subsystemSbandTarget",
            self.uhf_stack_target: "subsystemUhfTarget",
        }
        return mapping[unit]

    def run(self) -> dict[str, object]:
        state_before = self.collect_state()
        issues_before = self.analyze(state_before)
        repairs_performed: list[str] = []
        verdict = "ready"
        state_after = state_before
        issues_after = issues_before
        if issues_before:
            repairs_performed = self.perform_repairs(state_before, issues_before)
            state_after = self.collect_state()
            issues_after = self.analyze(state_after)
            if issues_after:
                verdict = "blocked"
            else:
                verdict = "repaired"

        if self.force_obc_comm_restart and verdict != "blocked":
            self.restart_service(self.obc_target, self.obc_service)
            repairs_performed.append(f"forced-restart:{self.obc_service}")
            time.sleep(self.repair_settle_sec)
            state_after = self.collect_state()
            issues_after = self.analyze(state_after)
            if issues_after:
                verdict = "blocked"
            else:
                verdict = "repaired"

        if verdict == "blocked" and any(
            issue.startswith("journal:obc-sband-availability-state:") for issue in issues_after
        ):
            self.perform_transport_reset(repairs_performed, state_after)
            state_after = self.collect_state()
            issues_after = self.analyze(state_after)
            verdict = "blocked" if issues_after else "repaired"
        target_beacon_sidecar = None
        if verdict != "blocked":
            try:
                repairs_before_sidecar = len(repairs_performed)
                target_beacon_sidecar = self.ensure_beacon_sidecar(repairs_performed)
                if len(repairs_performed) > repairs_before_sidecar:
                    time.sleep(self.repair_settle_sec)
                    state_after = self.collect_state()
                    issues_after = self.analyze(state_after)
                    verdict = "blocked" if issues_after else "repaired"
            except ProbeFailure as exc:
                issues_after = [*issues_after, f"target-beacon-sidecar:{exc}"]
                verdict = "blocked"
        return {
            "verdict": verdict,
            "requireUhfService": self.require_uhf_service,
            "ignoreGpsStateMissing": self.ignore_gps_state_missing,
            "forceObcCommRestart": self.force_obc_comm_restart,
            "expectedCommCanFdProfile": self.expected_comm_canfd_env,
            "problemsFound": issues_before,
            "repairsPerformed": repairs_performed,
            "stateBefore": state_before,
            "stateAfter": state_after,
            "remainingProblems": issues_after,
            "targetBeaconSidecar": target_beacon_sidecar,
        }


def emit_human_summary(summary: dict[str, object]) -> None:
    verdict = str(summary["verdict"]).upper()
    print(f"target-comm-lab-baseline: {verdict}")
    print(f"require-uhf-service={summary['requireUhfService']}")
    print(f"ignore-gps-state-missing={summary.get('ignoreGpsStateMissing', False)}")
    print(f"force-obc-comm-restart={summary.get('forceObcCommRestart', False)}")
    canfd_profile = summary.get("expectedCommCanFdProfile", {})
    print(
        "comm-canfd-profile="
        f"enabled:{canfd_profile.get('COMM_CSP_SOCKETCAN_USE_CANFD', '')},"
        f"destinations:{canfd_profile.get('COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST', '')},"
        f"dport:{canfd_profile.get('COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST', '')}"
    )
    problems = list(summary["problemsFound"])
    repairs = list(summary["repairsPerformed"])
    remaining = list(summary["remainingProblems"])
    if not problems and not repairs:
        print("no-action-needed=yes")
    else:
        print("problems-found:")
        if problems:
            for row in problems:
                print(f"  - {row}")
        else:
            print("  - none")
    if repairs:
        print("repairs-performed:")
        for row in repairs:
            print(f"  - {row}")
    if remaining:
        print("remaining-problems:")
        for row in remaining:
            print(f"  - {row}")


def main() -> int:
    args = parse_args()
    try:
        manager = TargetBaselineManager()
        summary = manager.run()
    except (KeyError, ValueError, ProbeFailure) as exc:
        payload = {
            "verdict": "blocked",
            "error": str(exc),
        }
        if args.json_out is not None:
            args.json_out.parent.mkdir(parents=True, exist_ok=True)
            args.json_out.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(f"target-comm-lab-baseline: BLOCKED\nerror={exc}", file=sys.stderr)
        return 1

    if args.json_out is not None:
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    emit_human_summary(summary)
    return 0 if summary["verdict"] != "blocked" else 1


if __name__ == "__main__":
    raise SystemExit(main())
