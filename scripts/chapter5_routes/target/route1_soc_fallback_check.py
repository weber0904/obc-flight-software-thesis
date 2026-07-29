#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import shlex
import sys
import time

ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
COMM_LIB_DIR = ROOT_DIR / "scripts" / "comm_verification" / "lib"
if str(COMM_LIB_DIR) not in sys.path:
    sys.path.insert(0, str(COMM_LIB_DIR))

import run_target_can_matrix_probe as target_probe
from run_target_can_matrix_probe import ProbeFailure, command_opcode, service_environment, service_process_environment, ssh_capture
from run_target_secure_auth_command_path_probe import SecureAuthCommandPathScenario


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    return parser.parse_args()


class Route1SocFallbackScenario(SecureAuthCommandPathScenario):
    BASELINE_SOC_PERCENT = 80.0

    def __init__(self, probe_root: pathlib.Path) -> None:
        super().__init__(probe_root)
        self.mode = "chapter5-route1-target-fallback"
        self.summary_path = self.diagnostics_dir / "route1-soc-fallback-summary.json"

    def load_route_opcodes(self) -> None:
        super().load_opcodes()
        dictionary = json.loads(self.dictionary_path.read_text(encoding="utf-8"))
        self.opcodes.update(
            {
                "OBCApp.modeManager.MODE_SET": command_opcode(dictionary, "OBCApp.modeManager.MODE_SET"),
                "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS": command_opcode(
                    dictionary, "OBCApp.payloadOpsController.PAYLOAD_GET_STATUS"
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

    def wait_journal(self, fragments: tuple[str, ...], timeout: float, label: str) -> None:
        journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
        self.wait_for_target_journal(self.obc_service, journal_since, fragments, timeout, label)

    def restore_eps_soc(self, socket_path: str, summary: list[str]) -> None:
        response = self.remote_set_soc(socket_path, self.BASELINE_SOC_PERCENT)
        summary.append(f"soc-restore-response={response}")
        self.checkpoint(
            "route1-eps-soc-restored",
            "pass",
            value=self.BASELINE_SOC_PERCENT,
            response=response,
        )

    def run(self) -> list[str]:
        summary: list[str] = []
        failure: dict[str, object] | None = None
        eps_socket: str | None = None
        eps_soc_changed = False
        restore_error: Exception | None = None
        try:
            self.begin_profile()
            self.record_secure_auth_provenance()
            self.load_route_opcodes()
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
            summary.append("case-sband-secure-auth=PASS")

            eps_soc_changed = True
            self.remote_set_soc(eps_socket, self.BASELINE_SOC_PERCENT)
            eps_get_opcode = self.opcodes["OBCApp.epsBridge.EPS_GET_STATUS"]
            _, eps_refresh_source = self.send_secure_command_name(
                self.sband,
                session,
                "route1 eps get status idle-payload precheck",
                "OBCApp.epsBridge.EPS_GET_STATUS",
                accept_sequence=True,
                journal_fragments=("EPS_STATUS_RECEIVED", f"Opcode 0x{eps_get_opcode:x} completed"),
                timeout=20.0,
            )
            summary.append(f"eps-refresh-source={eps_refresh_source}")
            _, mode_source = self.send_secure_command_name(
                self.sband,
                session,
                "route1 mode idle",
                "OBCApp.modeManager.MODE_SET",
                "IDLE",
                accept_sequence=True,
                journal_fragments=("SYS_MODE_CHANGE", "IDLE (1)"),
                timeout=20.0,
            )
            summary.append(f"mode-idle-source={mode_source}")
            _, payload_source = self.send_secure_command_name(
                self.sband,
                session,
                "route1 mode payload",
                "OBCApp.modeManager.MODE_SET",
                "PAYLOAD",
                accept_sequence=True,
                journal_fragments=("SYS_MODE_CHANGE", "PAYLOAD (3)"),
                timeout=20.0,
            )
            summary.append(f"mode-payload-source={payload_source}")

            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            response_59 = self.remote_set_soc(eps_socket, 59.0)
            summary.append(f"soc-59-response={response_59}")
            self.wait_for_target_journal(
                self.obc_service,
                journal_since,
                ("MODE_SAFETY_TRANSITION", "PAYLOAD (3) -> IDLE (1)"),
                20.0,
                "payload fallback to idle",
            )
            summary.append("case-payload-fallback-idle=PASS")

            _, reject_source = self.send_secure_command_name(
                self.sband,
                session,
                "route1 reject payload below threshold",
                "OBCApp.modeManager.MODE_SET",
                "PAYLOAD",
                accept_sequence=True,
                journal_fragments=("SYS_MODE_TRANSITION_REJECTED", "IDLE (1) to PAYLOAD (3)", "reason 4"),
                timeout=20.0,
            )
            summary.append(f"payload-reject-source={reject_source}")

            journal_since = ssh_capture(self.obc_target, "date '+%Y-%m-%d %H:%M:%S'").strip()
            response_39 = self.remote_set_soc(eps_socket, 39.0)
            summary.append(f"soc-39-response={response_39}")
            self.wait_for_target_journal(
                self.obc_service,
                journal_since,
                ("MODE_SAFETY_TRANSITION", "IDLE (1) -> SAFE (0)"),
                20.0,
                "idle fallback to safe",
            )
            summary.append("case-idle-fallback-safe=PASS")

            self.restore_eps_soc(eps_socket, summary)
            eps_soc_changed = False
            payload = {
                "mode": self.mode,
                "verdict": "PASS",
                "summary": summary,
            }
            self.write_json_artifact(self.summary_path, payload)
            summary.append(f"route1-soc-fallback-summary={self.summary_path}")
            summary.append("chapter5-route1-target-soc-fallback=PASS")
            return summary
        except Exception as exc:
            failure = {"error": str(exc)}
            raise
        finally:
            if eps_soc_changed and eps_socket is not None:
                try:
                    self.restore_eps_soc(eps_socket, summary)
                    eps_soc_changed = False
                except Exception as exc:
                    restore_error = exc
                    summary.append(f"soc-restore-error={exc}")
                    self.checkpoint(
                        "route1-eps-soc-restored",
                        "fail",
                        value=self.BASELINE_SOC_PERCENT,
                        error=str(exc),
                    )
                    if failure is not None:
                        failure["socRestoreError"] = str(exc)
            if failure is not None:
                self.write_json_artifact(
                    self.summary_path,
                    {
                        "mode": self.mode,
                        "verdict": "FAIL",
                        "failure": failure,
                        "summary": summary,
                    },
                )
            cleanup_run_error: Exception | None = None
            if failure is not None:
                cleanup_run_error = ProbeFailure(str(failure["error"]))
            elif restore_error is not None:
                cleanup_run_error = restore_error
            self.cleanup(cleanup_run_error)
            if restore_error is not None and failure is None:
                self.write_json_artifact(
                    self.summary_path,
                    {
                        "mode": self.mode,
                        "verdict": "FAIL",
                        "failure": {"error": f"EPS SoC restore failed: {restore_error}"},
                        "summary": summary,
                    },
                )
                raise ProbeFailure(f"EPS SoC restore failed: {restore_error}")


def main() -> int:
    args = parse_args()
    scenario = Route1SocFallbackScenario(pathlib.Path(args.probe_root))
    try:
        for line in scenario.run():
            print(line)
        return 0
    except Exception as exc:
        print(f"chapter5-route1-target-soc-fallback: FAIL {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
