#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import pathlib
import shlex
import sys
import time
import traceback

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
SCRIPTS_DIR = ROOT_DIR / "scripts"
if str(SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPTS_DIR))

from run_target_can_matrix_probe import (
    ProbeFailure,
    build_secure_command_v2_packet,
    current_service_journal,
    ensure_no_legacy_aliases,
    install_signal_cleanup,
    send_tts_raw_packet,
    service_environment,
    service_invocation_id,
    systemctl_show,
    wait_service_active,
)
from run_target_secure_auth_command_path_probe import SecureAuthCommandPathScenario
from route3_target_probe_common import (
    capture_target_status,
    current_metadata_path,
    read_boot_marker,
    read_metadata,
    require_env,
    run_secure_auth_command_path_subprobe,
    ssh_is_up,
    wait_for_ssh_down,
    wait_for_ssh_up,
    wait_current_invocation_ready,
    wait_watchdog_reboot_metadata,
)


def shq(value: str) -> str:
    return shlex.quote(value)


def watchdog_recovery_source_metadata(name: str) -> tuple[str, str]:
    normalized = name.strip().lower().replace("_", "-")
    mapping = {
        "eps-bridge": ("0", "WATCHDOG_EPS_BRIDGE"),
        "eps-fdir": ("1", "WATCHDOG_EPS_FDIR"),
        "mode-safety": ("2", "WATCHDOG_MODE_SAFETY"),
        "comm-controller": ("3", "WATCHDOG_COMM_CONTROLLER"),
        "adcs-fdir": ("5", "WATCHDOG_ADCS_FDIR"),
    }
    try:
        return mapping[normalized]
    except KeyError as exc:
        raise ProbeFailure(f"unsupported WATCHDOG_SOURCE {name!r}") from exc


def retry_once_after_ssh_glitch(label: str, action, retry_delay_sec: float = 30.0):
    try:
        return action()
    except ProbeFailure as exc:
        if "ssh command failed" not in str(exc):
            raise
        time.sleep(retry_delay_sec)
        try:
            return action()
        except ProbeFailure as retry_exc:
            raise ProbeFailure(f"{label} failed after one SSH reconnect retry: {retry_exc}") from retry_exc


class SecureAuthWatchdogResetScenario(SecureAuthCommandPathScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.require_uhf = os.getenv("TARGET_BASELINE_REQUIRE_UHF_SERVICE", "1") == "1"
        self.watchdog_source = require_env("WATCHDOG_SOURCE")
        self.watchdog_source_code, self.watchdog_source_name = watchdog_recovery_source_metadata(self.watchdog_source)
        self.service_log = self.probe_root / "service-status.log"
        self.metadata_path = ""
        self.baseline_boot_count = 0
        self.baseline_boot_marker = ""
        self.baseline_pid = "0"
        self.baseline_restarts = "0"

    def load_opcodes(self) -> None:
        super().load_opcodes()
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS": next(
                    int(entry["opcode"])
                    for entry in dictionary.get("commands", [])
                    if entry.get("name") == "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS"
                ),
                "OBCApp.watchdogSupervisor.SET_WATCHDOG_PROBE_SUPPRESSION": next(
                    int(entry["opcode"])
                    for entry in dictionary.get("commands", [])
                    if entry.get("name") == "OBCApp.watchdogSupervisor.SET_WATCHDOG_PROBE_SUPPRESSION"
                ),
                "OBCApp.watchdogSupervisor.GET_WATCHDOG_STATUS": next(
                    int(entry["opcode"])
                    for entry in dictionary.get("commands", [])
                    if entry.get("name") == "OBCApp.watchdogSupervisor.GET_WATCHDOG_STATUS"
                ),
            }
        )

    def capture_status(self, label: str) -> None:
        capture_target_status(
            obc_target=self.obc_target,
            obc_service=self.obc_service,
            metadata_path=self.metadata_path,
            service_log=self.service_log,
            label=label,
        )

    def wait_for_reboot_edge(self, timeout: int, initial_invocation_id: str) -> str:
        deadline = time.time() + timeout
        last_journal = ""
        while time.time() < deadline:
            if not ssh_is_up(self.obc_target):
                return "target-reboot-edge"
            try:
                current_invocation = service_invocation_id(self.obc_target, self.obc_service)
                last_journal = current_service_journal(self.obc_target, self.obc_service, fallback_lines=400)
            except ProbeFailure as exc:
                if "ssh command failed" in str(exc):
                    if not ssh_is_up(self.obc_target):
                        return "target-reboot-edge"
                    time.sleep(0.5)
                    continue
                raise
            if (
                "WATCHDOG_PROBE_SUPPRESSION_UPDATED" in last_journal
                and "RECOVERY_INCIDENT_OPENED" in last_journal
                and self.watchdog_source_name in last_journal
                and "R2_RESTART_SOFTWARE_COMPONENT" in last_journal
            ):
                return "target-journal"
            if (
                current_invocation
                and current_invocation != initial_invocation_id
                and
                "BOOT_RECOVERY_STATUS" in last_journal
                and self.watchdog_source_name in last_journal
                and "R6_OBC_REBOOT" in last_journal
            ):
                return "post-reboot-boot-status"
            if not ssh_is_up(self.obc_target):
                return "target-reboot-edge"
            time.sleep(0.5)
        raise ProbeFailure(
            "timed out waiting for watchdog suppression to reach initial recovery transition; "
            f"last journal tail={last_journal[-4000:]}"
        )

    def send_watchdog_trigger(self, session) -> tuple[int, str]:
        self.prepare_ground_window(self.sband, timeout=8.0)
        time.sleep(0.3)
        sequence = session.accept_sequence()
        initial_invocation_id = service_invocation_id(self.obc_target, self.obc_service)
        inner = self.encode_inner_command(
            "OBCApp.watchdogSupervisor.SET_WATCHDOG_PROBE_SUPPRESSION",
            self.watchdog_source.upper().replace("-", "_"),
            "true",
        )
        payload = build_secure_command_v2_packet(inner, session.session_key, sequence)
        with self.sband.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(
                "sband-secure-watchdog-probe-suppression: "
                f"command=OBCApp.watchdogSupervisor.SET_WATCHDOG_PROBE_SUPPRESSION "
                f"wdSource={self.watchdog_source} seq={sequence} outer={payload.hex()}\n"
            )
        send_tts_raw_packet(self.sband.gds_tts_port, payload)
        source = self.wait_for_reboot_edge(timeout=20, initial_invocation_id=initial_invocation_id)
        self.checkpoint(
            "sband-secure-watchdog-probe-suppression",
            "pass",
            sequence=sequence,
            source=source,
            watchdog_source=self.watchdog_source,
        )
        return sequence, source

    def run_probe(self) -> list[str]:
        baseline_env = service_environment(self.obc_target, self.obc_service)
        if baseline_env.get("HARDWARE_WATCHDOG", "") == "disabled":
            raise ProbeFailure(
                "installed target service has HARDWARE_WATCHDOG=disabled; current Route 3 watchdog board-reset proof "
                "requires the maintained target baseline to run with linux-device ownership instead of an external "
                "disable override"
            )

        self.metadata_path, baseline_env = current_metadata_path(
            self.obc_target,
            self.obc_service,
            os.getenv("RUNTIME_ROOT", ""),
        )
        self.baseline_boot_marker = read_boot_marker(self.obc_target)
        baseline_metadata = read_metadata(self.obc_target, self.metadata_path)
        self.baseline_boot_count = int(baseline_metadata.get("boot_count", "0") or "0")
        baseline_service = systemctl_show(self.obc_target, self.obc_service, ("MainPID", "NRestarts", "ActiveState"))
        self.baseline_pid = baseline_service.get("MainPID", "0")
        self.baseline_restarts = baseline_service.get("NRestarts", "0")

        self.capture_status("baseline")

        self.begin_profile()
        self.record_secure_auth_provenance()
        self.wait_for_target_ready_for_comm(self.require_uhf)
        self.capture_status("current-baseline-ready")

        self.apply_sband_ingress_diagnostics_override()
        self.start_security_server()
        self.wait_for_sband_tcp_reachability(10.0)
        self.start_ground_paths(need_sband=True, need_uhf=False)
        readiness = self.ensure_sband_ground_ready()
        self.prepare_ground_window(self.sband, timeout=8.0)
        self.checkpoint("sband-ground-readiness-reused", "pass", readiness=readiness)

        session = self.authenticate_secure_service(
            self.sband,
            service_id=1,
            ingress_port=0,
            role_fragment="identity 1 role 1",
            initial_sequence=41,
        )
        hw_watchdog_oracle = self.build_ground_event_readback_oracle(
            event_log_path=self.sband.native_event_log,
            event_offset_attr="native_event_log_offset",
            ground_fragments=("HW_WATCHDOG_STATUS", "timeout 15"),
            source_name="ground-native-event-log",
        )
        status_outcome = self.send_secure_command_until_ground_readback(
            self.sband,
            session,
            "sband-secure-get-hw-watchdog-status",
            "OBCApp.linuxWatchdogSink.GET_HW_WATCHDOG_STATUS",
            accept_sequence=True,
            journal_fragments=("HW_WATCHDOG_STATUS", "timeout 15"),
            ground_success_oracle=hw_watchdog_oracle,
            attempt_limit=30,
            per_attempt_timeout=1.0,
        )
        status_sequence = int(status_outcome["acceptedSequence"])
        status_source = str(status_outcome["source"])
        trigger_sequence, trigger_source = self.send_watchdog_trigger(session)

        if trigger_source not in ("target-reboot-edge", "post-reboot-boot-status"):
            wait_for_ssh_down(self.obc_target, int(require_env("SSH_DOWN_TIMEOUT_SEC")))
        wait_for_ssh_up(self.obc_target, int(require_env("REBOOT_TIMEOUT_SEC")))
        retry_once_after_ssh_glitch(
            "post-reboot service active wait",
            lambda: wait_service_active(
                self.obc_target,
                self.obc_service,
                int(require_env("SERVICE_ACTIVE_TIMEOUT_SEC")),
            ),
        )
        retry_once_after_ssh_glitch(
            "post-reboot target readiness wait",
            lambda: wait_current_invocation_ready(scenario=self, require_uhf=self.require_uhf),
        )

        rebooted_service = retry_once_after_ssh_glitch(
            "post-reboot service snapshot",
            lambda: systemctl_show(self.obc_target, self.obc_service, ("MainPID", "NRestarts", "ActiveState")),
        )
        rebooted_boot_marker = retry_once_after_ssh_glitch(
            "post-reboot boot marker read",
            lambda: read_boot_marker(self.obc_target),
        )
        if rebooted_boot_marker == self.baseline_boot_marker:
            raise ProbeFailure(
                "hardware watchdog reset proof did not change the target boot marker: "
                f"before={self.baseline_boot_marker} after={rebooted_boot_marker}"
            )

        metadata = retry_once_after_ssh_glitch(
            "post-reboot watchdog metadata wait",
            lambda: wait_watchdog_reboot_metadata(
                target=self.obc_target,
                metadata_path=self.metadata_path,
                baseline_boot_count=self.baseline_boot_count,
                recovery_source_code=self.watchdog_source_code,
                timeout=int(require_env("SERVICE_ACTIVE_TIMEOUT_SEC")),
            ),
        )

        self.sband.stop()
        self.checkpoint("pre-reboot-ground-helpers-stopped", "pass")
        post_stdout, post_summary = retry_once_after_ssh_glitch(
            "post-reboot secure-auth subprobe",
            lambda: run_secure_auth_command_path_subprobe(
                root_dir=ROOT_DIR,
                probe_root=self.probe_root / "post-r6-secure-auth",
                require_uhf=self.require_uhf,
                extra_readbacks=("hw-watchdog-status", "watchdog-status", "persistent-fault-history"),
            ),
        )
        self.capture_status("after-hardware-watchdog-reboot")
        self.capture_status("post-reboot-secure-auth-restored")

        return [
            "rpi target hardware watchdog reset probe PASS",
            f"obc-service={self.obc_service} target={self.obc_target}",
            f"watchdog-source={self.watchdog_source}",
            f"baseline-main-pid={self.baseline_pid} rebooted-main-pid={rebooted_service.get('MainPID', '0')}",
            f"baseline-restarts={self.baseline_restarts} rebooted-restarts={rebooted_service.get('NRestarts', '0')}",
            f"baseline-boot-marker={self.baseline_boot_marker}",
            f"rebooted-boot-marker={rebooted_boot_marker}",
            "reset_cause=RECOVERY_WATCHDOG",
            f"last_recovery_source={self.watchdog_source_name}",
            "last_recovery_level=R6_OBC_REBOOT",
            (
                "command_path="
                f"secure-auth bootstrap=PASS; "
                f"pre-reset GET_HW_WATCHDOG_STATUS seq={status_sequence} via {status_source}; "
                f"trigger seq={trigger_sequence} via {trigger_source}; "
                f"post-reboot secure-auth summary={post_summary}; "
                f"post-reboot stdout={post_stdout.replace(chr(10), ' | ')}"
            ),
            f"boot_count={metadata.get('boot_count')}",
            f"consecutive_reset_count={metadata.get('consecutive_reset_count')}",
            "service_restore=maintained-nonquiet-baseline",
            f"log={self.service_log}",
            f"command-log={self.sband.raw_command_log}",
            f"events-log={self.sband.events_log}",
        ]


def main() -> int:
    probe_root = pathlib.Path(require_env("PROBE_TMP_DIR"))
    scenario = SecureAuthWatchdogResetScenario(probe_root)
    install_signal_cleanup(lambda: (scenario.sband.force_stop(), scenario.uhf.force_stop()))
    exit_code = 0
    output = ""
    run_error: Exception | None = None
    try:
        scenario.start()
        summary_lines = scenario.run_probe()
        ensure_no_legacy_aliases(ROOT_DIR)
        output = "\n".join(summary_lines) + "\n"
    except ProbeFailure as exc:
        run_error = exc
        output = f"rpi target hardware watchdog reset probe FAIL {exc}\n"
        exit_code = 1
    except Exception as exc:  # pragma: no cover - top-level trap
        run_error = exc
        output = "rpi target hardware watchdog reset probe FAIL unexpected exception\n" + traceback.format_exc()
        exit_code = 1
    finally:
        try:
            scenario.cleanup(run_error)
        except Exception as cleanup_exc:  # pragma: no cover - best effort
            if exit_code == 0:
                output = f"rpi target hardware watchdog reset probe FAIL cleanup {cleanup_exc}\n"
                exit_code = 1
            else:
                output += f"\ncleanup-error: {cleanup_exc}\n"
        scenario.sband.force_stop()
        scenario.uhf.force_stop()
        if exit_code == 0:
            sys.stdout.write(output)
        else:
            sys.stderr.write(output)
        sys.stdout.flush()
        sys.stderr.flush()
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
