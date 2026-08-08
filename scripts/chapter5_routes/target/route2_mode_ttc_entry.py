#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import shlex
import sys
import time

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
COMM_LIB_DIR = ROOT_DIR / "scripts" / "comm_verification" / "lib"
if str(COMM_LIB_DIR) not in sys.path:
    sys.path.insert(0, str(COMM_LIB_DIR))

import run_target_can_matrix_probe as target_probe
from run_target_can_matrix_probe import (
    ProbeFailure,
    apply_service_override,
    command_opcode,
    remove_service_override,
    service_environment,
    service_process_environment,
    ssh_capture,
    wait_service_active,
)
from run_target_secure_auth_command_path_probe import SecureAuthCommandPathScenario


GPS_OVERRIDE_DROPIN = "59-chapter5-ttc-gps-replay.conf"
GPS_REMOTE_REPLAY = "/tmp/chapter5-route2-ttc-replay.nmea"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    return parser.parse_args()


class Route2ModeTtcEntryScenario(SecureAuthCommandPathScenario):
    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.mode = "chapter5-route2-target-ttc"
        self.summary_path = self.diagnostics_dir / "route2-mode-ttc-entry-summary.json"
        self.gps_override_applied = False

    def load_route_opcodes(self) -> None:
        super().load_opcodes()
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.modeManager.MODE_SET": command_opcode(dictionary, "OBCApp.modeManager.MODE_SET"),
                "OBCApp.ttcPassManager.TTC_SET_POLICY": command_opcode(dictionary, "OBCApp.ttcPassManager.TTC_SET_POLICY"),
                "OBCApp.ttcPassManager.TTC_SET_PASS_WINDOW": command_opcode(dictionary, "OBCApp.ttcPassManager.TTC_SET_PASS_WINDOW"),
                "OBCApp.adcsBridge.ADCS_SET_MODE": command_opcode(dictionary, "OBCApp.adcsBridge.ADCS_SET_MODE"),
                "OBCApp.adcsBridge.ADCS_GET_ATTITUDE": command_opcode(
                    dictionary, "OBCApp.adcsBridge.ADCS_GET_ATTITUDE"
                ),
            }
        )

    def resolve_eps_control_socket(self) -> str:
        for env_source in (
            service_environment(self.subsystem_target, self.eps_service),
            service_process_environment(self.subsystem_target, self.eps_service),
        ):
            value = env_source.get("EPS_SIM_CONTROL_SOCKET", "").strip()
            if value:
                return value
        raise ProbeFailure(
            f"{self.eps_service} on {self.subsystem_target} does not expose EPS_SIM_CONTROL_SOCKET; "
            "install the updated subsystem EPS service baseline first"
        )

    def remote_set_soc(self, socket_path: str, value: float, transition_sec: float = 0.0) -> str:
        script = """
import socket
import sys

socket_path = sys.argv[1]
command = f"set-soc {float(sys.argv[2]):.2f} {float(sys.argv[3]):.2f}\\n".encode("utf-8")
sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.settimeout(5.0)
sock.connect(socket_path)
sock.sendall(command)
response = sock.recv(4096).decode("utf-8", errors="replace").strip()
sock.close()
print(response)
if not response.startswith("OK "):
    raise SystemExit(2)
""".strip()
        return ssh_capture(
            self.subsystem_target,
            f"python3 -c {shlex.quote(script)} {shlex.quote(socket_path)} {value:.2f} {transition_sec:.2f}",
        ).strip()

    def install_gps_replay_override(self) -> None:
        replay_payload = self.build_replay_nmea()
        write_result = target_probe.subprocess.run(
            target_probe.ssh_command(self.obc_target, f"cat > {shlex.quote(GPS_REMOTE_REPLAY)}"),
            input=replay_payload,
            capture_output=True,
            text=True,
            check=False,
        )
        if write_result.returncode != 0:
            raise ProbeFailure(
                f"failed to stage GPS replay file on {self.obc_target}: rc={write_result.returncode}\n"
                f"stdout={write_result.stdout}\nstderr={write_result.stderr}"
            )
        apply_service_override(
            self.obc_target,
            self.obc_service,
            GPS_OVERRIDE_DROPIN,
            {
                "OBC_GPS_SOURCE_MODE": "replay",
                "OBC_GPS_REPLAY_FILE": GPS_REMOTE_REPLAY,
            },
        )
        self.gps_override_applied = True
        wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
        self.wait_for_target_ready_for_comm(True)

    def begin_profile(self) -> None:
        super().begin_profile()
        self.wait_for_target_ready_for_comm(True)

    def remove_gps_replay_override(self) -> None:
        if self.gps_override_applied:
            remove_service_override(self.obc_target, self.obc_service, GPS_OVERRIDE_DROPIN)
            wait_service_active(self.obc_target, self.obc_service, self.restart_timeout)
            self.gps_override_applied = False
        ssh_capture(self.obc_target, f"rm -f {shlex.quote(GPS_REMOTE_REPLAY)}", check=False)

    @staticmethod
    def build_replay_nmea() -> str:
        def make_sentence(payload: str) -> str:
            checksum = 0
            for ch in payload:
                checksum ^= ord(ch)
            return f"${payload}*{checksum:02X}"

        remote_epoch = int(time.time()) + 2
        lines: list[str] = []
        for step in range(180):
            moment = time.gmtime(remote_epoch + step)
            hhmmss = time.strftime("%H%M%S", moment)
            ddmmyy = time.strftime("%d%m%y", moment)
            payload = f"GPRMC,{hhmmss}.00,A,2503.7135,N,12133.5335,E,0.0,0.0,{ddmmyy},0.0,E"
            lines.append(make_sentence(payload))
        return "\n".join(lines) + "\n"

    @staticmethod
    def parse_u64_channel(snapshot: dict[str, object], channel_name: str) -> int:
        matches = [str(line) for line in snapshot.get("matches", [])]
        for line in matches:
            numbers = re.findall(r"(\d+)", line)
            if numbers:
                return int(numbers[-1])
        raise ProbeFailure(f"could not parse {channel_name} from channel snapshot: {snapshot}")

    def wait_ground_channel_value(
        self,
        label: str,
        search: str,
        expected_tokens: tuple[str, ...],
        timeout_sec: float = 15.0,
    ) -> dict[str, object]:
        deadline = time.time() + timeout_sec
        last_snapshot: dict[str, object] | None = None
        expected = {token.casefold() for token in expected_tokens}
        while time.time() < deadline:
            snapshot = self.search_channel_snapshot(self.sband, label, search)
            last_snapshot = snapshot
            for raw_line in snapshot.get("matches", []):
                line = str(raw_line).strip()
                if not line:
                    continue
                value = line.rsplit(",", maxsplit=1)[-1].strip() if "," in line else line.rsplit(maxsplit=1)[-1]
                if value.casefold() in expected:
                    return snapshot
            time.sleep(0.5)
        raise ProbeFailure(
            f"ground {search} query did not show any of {expected_tokens}: {last_snapshot}"
        )

    def wait_ground_event_fragments(
        self,
        label: str,
        event_start: int,
        expected_fragments: tuple[str, ...],
        timeout_sec: float = 15.0,
    ) -> str:
        deadline = time.time() + timeout_sec
        last_events = ""
        while time.time() < deadline:
            try:
                lines = self.sband.events_log.read_text(encoding="utf-8", errors="replace").splitlines()
            except FileNotFoundError:
                lines = []
            last_events = "\n".join(lines[event_start:])
            if all(fragment in last_events for fragment in expected_fragments):
                return "ground-events"
            time.sleep(0.5)
        raise ProbeFailure(
            f"{label} did not appear in ground events with fragments {expected_fragments}: {last_events[-4000:]}"
        )

    def record_ground_channel_best_effort(
        self,
        summary: list[str],
        *,
        label: str,
        search: str,
        expected_tokens: tuple[str, ...],
        timeout_sec: float = 10.0,
    ) -> None:
        try:
            snapshot = self.wait_ground_channel_value(label, search, expected_tokens, timeout_sec=timeout_sec)
            summary.append(f"{label}={expected_tokens[0]}")
            summary.append(f"{label}-snapshot={snapshot}")
        except ProbeFailure as exc:
            summary.append(f"{label}=BEST_EFFORT_MISS")
            summary.append(f"{label}-detail={exc}")

    def run(self) -> list[str]:
        summary: list[str] = []
        failure: dict[str, object] | None = None
        try:
            self.begin_profile()
            self.record_secure_auth_provenance()
            self.load_route_opcodes()
            self.install_gps_replay_override()
            eps_socket = self.resolve_eps_control_socket()
            summary.append(f"eps-control-socket={eps_socket}")

            self.start_security_server()
            self.wait_for_sband_tcp_reachability(10.0)
            self.start_ground_paths(need_sband=True, need_uhf=False)
            readiness = self.ensure_sband_ground_ready()
            self.prepare_ground_window(self.sband, timeout=8.0)
            summary.append(f"sband-ground-readiness={readiness}")

            session = self.authenticate_secure_service(
                self.sband,
                service_id=1,
                ingress_port=0,
                role_fragment="identity 1 role 1",
                initial_sequence=41,
            )
            session_source = "secure-auth-v2"
            summary.append(f"sband-session-source={session_source}")

            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            summary.append(f"soc-09-response={self.remote_set_soc(eps_socket, 9.0)}")
            self.wait_for_target_journal(
                self.obc_service,
                journal_since,
                ("MODE_SAFETY_TRANSITION", "SAFE (0) -> HELL (2)"),
                20.0,
                "safe to hell",
            )
            summary.append("case-safe-to-hell=PASS")

            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            summary.append(f"soc-16-response={self.remote_set_soc(eps_socket, 16.0)}")
            self.wait_for_target_journal(
                self.obc_service,
                journal_since,
                ("MODE_SAFETY_TRANSITION", "HELL (2) -> SAFE (0)"),
                20.0,
                "hell to safe",
            )
            summary.append("case-hell-to-safe=PASS")

            summary.append(f"soc-60-response={self.remote_set_soc(eps_socket, 60.0)}")
            eps_get_opcode = self.opcodes["OBCApp.epsBridge.EPS_GET_STATUS"]
            _, eps_refresh_source = self.send_secure_command_name(
                self.sband,
                session,
                "route2 eps get status safe-idle precheck",
                "OBCApp.epsBridge.EPS_GET_STATUS",
                accept_sequence=True,
                journal_fragments=("EPS_STATUS_RECEIVED", f"Opcode 0x{eps_get_opcode:x} completed"),
                timeout=20.0,
            )
            summary.append(f"eps-refresh-source={eps_refresh_source}")
            _, idle_source = self.send_secure_command_name(
                self.sband,
                session,
                "route2 mode idle",
                "OBCApp.modeManager.MODE_SET",
                "IDLE",
                accept_sequence=True,
                journal_fragments=("SYS_MODE_CHANGE", "IDLE (1)"),
                timeout=20.0,
            )
            summary.append(f"idle-source={idle_source}")

            adcs_set_idle_opcode = self.opcodes["OBCApp.adcsBridge.ADCS_SET_MODE"]
            _, adcs_idle_source = self.send_secure_command_name(
                self.sband,
                session,
                "route2 adcs set idle",
                "OBCApp.adcsBridge.ADCS_SET_MODE",
                "IDLE",
                accept_sequence=True,
                journal_fragments=(f"Opcode 0x{adcs_set_idle_opcode:x} completed",),
                timeout=20.0,
            )
            summary.append(f"adcs-set-idle-source={adcs_idle_source}")

            adcs_get_opcode = self.opcodes["OBCApp.adcsBridge.ADCS_GET_ATTITUDE"]
            _, adcs_idle_readback_source = self.send_secure_command_name(
                self.sband,
                session,
                "route2 adcs get attitude idle precheck",
                "OBCApp.adcsBridge.ADCS_GET_ATTITUDE",
                accept_sequence=True,
                journal_fragments=(f"Opcode 0x{adcs_get_opcode:x} completed",),
                timeout=20.0,
            )
            summary.append(f"adcs-idle-readback-source={adcs_idle_readback_source}")
            self.record_ground_channel_best_effort(
                summary,
                label="ground-adcs-mode-precheck",
                search="ADCS_MODE",
                expected_tokens=("IDLE",),
                timeout_sec=15.0,
            )

            _, policy_source = self.send_secure_command_name(
                self.sband,
                session,
                "route2 ttc set policy",
                "OBCApp.ttcPassManager.TTC_SET_POLICY",
                "true",
                "10",
                accept_sequence=True,
                journal_fragments=("TTC_POLICY_CONFIG_UPDATED", "enabled 1", "loss timeout 10"),
                timeout=20.0,
            )
            summary.append(f"ttc-policy-source={policy_source}")

            gps_unix_snapshot = self.search_channel_snapshot(
                self.sband,
                "route2-ttc-current-gps-unix",
                "TTC_POLICY_CURRENT_GPS_UNIX_SEC",
            )
            try:
                gps_unix = self.parse_u64_channel(gps_unix_snapshot, "TTC_POLICY_CURRENT_GPS_UNIX_SEC")
                summary.append(f"ttc-current-gps-unix={gps_unix}")
            except ProbeFailure as exc:
                gps_unix = int(time.time())
                summary.append(f"ttc-current-gps-unix=HOST_TIME_FALLBACK {gps_unix}")
                summary.append(f"ttc-current-gps-unix-detail={exc}")

            pointing_event_start = self.sband.event_count()
            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()

            _, window_source = self.send_secure_command_name(
                self.sband,
                session,
                "route2 ttc set window",
                "OBCApp.ttcPassManager.TTC_SET_PASS_WINDOW",
                str(max(0, gps_unix - 30)),
                str(gps_unix + 120),
                accept_sequence=True,
                journal_fragments=("TTC_PASS_WINDOW_SET",),
                timeout=20.0,
            )
            summary.append(f"ttc-window-source={window_source}")
            self.wait_for_target_journal(
                self.obc_service,
                journal_since,
                ("TTC_POLICY_ENTRY_REQUEST", "TTC policy requested TTC entry reason 1"),
                60.0,
                "ttc entry request",
            )
            summary.append("ttc-entry-request-source=target-journal")
            self.wait_for_target_journal(
                self.obc_service,
                journal_since,
                ("SYS_MODE_CHANGE", "TTC (4)"),
                60.0,
                "ttc mode entry",
            )
            self.wait_for_target_journal(
                self.obc_service,
                journal_since,
                ("ADCS_MODE_CHANGE", "POINTING (2)"),
                60.0,
                "adcs pointing hook",
            )
            self.wait_for_target_journal(
                self.obc_service,
                journal_since,
                ("ADCS_POINTING_ACQUIRED",),
                60.0,
                "adcs pointing acquired",
            )

            _, adcs_get_source = self.send_secure_command_name(
                self.sband,
                session,
                "route2 adcs get attitude",
                "OBCApp.adcsBridge.ADCS_GET_ATTITUDE",
                accept_sequence=True,
                journal_fragments=(f"Opcode 0x{adcs_get_opcode:x} completed",),
                timeout=20.0,
            )
            summary.append(f"adcs-get-attitude-source={adcs_get_source}")
            try:
                ground_pointing_source = self.wait_ground_event_fragments(
                    "route2-ground-adcs-pointing",
                    pointing_event_start,
                    ("ADCS_MODE_CHANGE", "POINTING"),
                    timeout_sec=15.0,
                )
                summary.append(f"ground-adcs-pointing-source={ground_pointing_source}")
            except ProbeFailure as exc:
                summary.append(f"ground-adcs-pointing-source=BEST_EFFORT_MISS detail={exc}")

            self.record_ground_channel_best_effort(
                summary,
                label="ground-adcs-mode-final",
                search="ADCS_MODE",
                expected_tokens=("POINTING",),
                timeout_sec=15.0,
            )
            adcs_snapshot = self.search_channel_snapshot(self.sband, "route2-adcs-mode-best-effort", "ADCS_MODE", timeout_sec=8)
            summary.append(f"ground-adcs-mode-best-effort={adcs_snapshot}")

            payload = {
                "mode": self.mode,
                "verdict": "PASS",
                "summary": summary,
                "groundAdcsSnapshot": adcs_snapshot,
            }
            self.write_json_artifact(self.summary_path, payload)
            summary.append(f"route2-mode-ttc-entry-summary={self.summary_path}")
            summary.append("chapter5-route2-target-mode-ttc-entry=PASS")
            return summary
        except Exception as exc:
            failure = {"error": str(exc)}
            raise
        finally:
            gps_cleanup_error: str | None = None
            cleanup_run_error = None if failure is None else ProbeFailure(str(failure["error"]))
            try:
                self.remove_gps_replay_override()
            except Exception as exc:
                gps_cleanup_error = str(exc)
                summary.append(f"gps-replay-override-cleanup=BEST_EFFORT_FAIL detail={exc}")
            if failure is not None:
                payload = {"mode": self.mode, "verdict": "FAIL", "failure": failure, "summary": summary}
                if gps_cleanup_error is not None:
                    payload["cleanupError"] = gps_cleanup_error
                self.write_json_artifact(self.summary_path, payload)
            self.cleanup(cleanup_run_error)
            if failure is None and gps_cleanup_error is not None:
                payload = {
                    "mode": self.mode,
                    "verdict": "FAIL",
                    "failure": {"error": f"gps replay override cleanup failed: {gps_cleanup_error}"},
                    "summary": summary,
                    "cleanupError": gps_cleanup_error,
                }
                self.write_json_artifact(self.summary_path, payload)
                raise ProbeFailure(f"gps replay override cleanup failed: {gps_cleanup_error}")


def main() -> int:
    args = parse_args()
    scenario = Route2ModeTtcEntryScenario(pathlib.Path(args.probe_root))
    try:
        for line in scenario.run():
            print(line)
        return 0
    except Exception as exc:
        print(f"chapter5-route2-target-mode-ttc-entry: FAIL {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
