#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import pathlib
import re
import shutil
import struct
import sys
import time
from collections import Counter
from dataclasses import dataclass

from secure_link_auth_lib import (
    AuthStatusCode,
    HandshakeMessageType,
    SERVICE_ID_SBAND,
    build_inner_command,
    build_req_auth_packet,
    build_response_packet,
    build_secure_command_v2_packet,
    compute_auth_response,
    load_handshake_messages,
    request_session_key,
    send_tts_raw_packet,
    wait_for_handshake_message,
)


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if not value:
        raise RuntimeError(f"missing required environment variable: {name}")
    return value


def read_text(path: pathlib.Path) -> str:
    try:
        return path.read_bytes().replace(b"\0", b"\n").decode("utf-8", errors="replace")
    except FileNotFoundError:
        return ""


def file_size(path: pathlib.Path) -> int:
    try:
        return path.stat().st_size
    except FileNotFoundError:
        return 0


def read_text_since(path: pathlib.Path, offset: int) -> str:
    try:
        payload = path.read_bytes()
    except FileNotFoundError:
        return ""
    bounded_offset = max(0, min(offset, len(payload)))
    return payload[bounded_offset:].replace(b"\0", b"\n").decode("utf-8", errors="replace")


def wait_text_since(path: pathlib.Path, offset: int, fragment: str, timeout: float) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text_since(path, offset)
        if fragment in text:
            return text
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for {fragment!r} in {path} from offset {offset}")


def wait_regex_since(path: pathlib.Path, offset: int, pattern: str, timeout: float) -> str:
    compiled = re.compile(pattern)
    deadline = time.time() + timeout
    while time.time() < deadline:
        text = read_text_since(path, offset)
        if compiled.search(text) is not None:
            return text
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for regex {pattern!r} in {path} from offset {offset}")


def parse_ground_runtime_marker(line: str) -> tuple[int, int] | None:
    match = re.search(r"\(\d+\(\d+\)-(\d+):(\d+)\)", line)
    if match is None:
        return None
    return int(match.group(1)), int(match.group(2))


def parse_obc_runtime_marker(line: str) -> tuple[int, int] | None:
    match = re.search(r"\(\d+:(\d+),(\d+)\)", line)
    if match is None:
        return None
    return int(match.group(1)), int(match.group(2))


def find_channel_search_hit(text: str, search: str) -> str | None:
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("$ ") or line.startswith("returncode="):
            continue
        if search in line:
            return line
    return None


def wait_stable_size(path: pathlib.Path, expected_size: int, quiet_sec: float, timeout: float) -> None:
    deadline = time.time() + timeout
    quiet_start = time.time()
    last_size = expected_size
    while time.time() < deadline:
        current = file_size(path)
        if current != last_size:
            last_size = current
            quiet_start = time.time()
            time.sleep(0.2)
            continue
        if (time.time() - quiet_start) >= quiet_sec:
            return
        time.sleep(0.2)
    raise RuntimeError(f"timed out waiting for quiet stable size on {path}")


def clean_dir(path: pathlib.Path) -> None:
    resolved = path.resolve()
    if str(resolved) in {"/", "/tmp"}:
        raise RuntimeError(f"refusing to remove unsafe directory: {resolved}")
    if resolved.exists():
        shutil.rmtree(resolved)
    resolved.mkdir(parents=True, exist_ok=True)


def dictionary_command_opcode(dictionary: dict[str, object], name: str) -> int:
    for entry in dictionary.get("commands", []):
        if entry.get("name") == name:
            return int(entry["opcode"])
    raise RuntimeError(f"command {name!r} not found in dictionary")


@dataclass
class SecureSession:
    service_id: int
    session_key: bytes
    next_sequence: int = 41

    def accept_sequence(self) -> int:
        sequence = self.next_sequence
        self.next_sequence += 1
        return sequence


ROOT_DIR = pathlib.Path(require_env("ROOT_DIR"))
BIN_DIR = pathlib.Path(require_env("BIN_DIR"))
DICT_PATH = pathlib.Path(require_env("DICT_PATH"))
PROBE_TMP_DIR = pathlib.Path(require_env("PROBE_TMP_DIR"))
RUNTIME_ROOT = pathlib.Path(require_env("RUNTIME_ROOT"))
PYTHON_BIN = require_env("PYTHON_BIN")

SECURITY_SERVER_SOCKET = PROBE_TMP_DIR / "security-server" / "secure-server.sock"
SECURITY_SERVER_LOG = PROBE_TMP_DIR / "security-server.log"
HOSTED_TIMEOUT_SEC = 30.0
AUTH_TIMEOUT_SEC = 10.0
AUTH_ESTABLISH_TIMEOUT_SEC = 30.0
AUTH_RETRY_BACKOFF_SEC = 0.5
QUIET_WINDOW_SEC = 3.0
COMM_BAND_UHF = 1

sys.path.insert(0, str(ROOT_DIR / "scripts"))
from decode_ccsds_capture import packet_summary, parse_tm_frames  # noqa: E402
from per_band_stock_ground_stacks import GroundPath, HostedPerBandStockStacks, find_fprime_cli  # noqa: E402
from probe_process_utils import cleanup_managed_processes, start_managed_process  # noqa: E402

LIVE_DOWNLINK_APIDS = (1, 2, 4)


class HostedObservabilityGovernanceProbe:
    def __init__(self) -> None:
        self.probe_root = PROBE_TMP_DIR
        self.stack_root = self.probe_root / "combined-stack"
        self.runtime_root = RUNTIME_ROOT
        self.dictionary = json.loads(DICT_PATH.read_text(encoding="utf-8"))
        self.processes = []
        self.stack: HostedPerBandStockStacks | None = None
        self.sband: GroundPath | None = None
        self.uhf: GroundPath | None = None
        self.obc_log = self.stack_root / "logs" / "obc.log"
        self.summary: list[str] = []

        self.opcode_get_reset_cause = dictionary_command_opcode(self.dictionary, "OBCApp.bootManager.GET_RESET_CAUSE")
        self.opcode_comm_set_active = dictionary_command_opcode(self.dictionary, "OBCApp.commController.COMM_SET_ACTIVE")
        self.opcode_eps_get_status = dictionary_command_opcode(self.dictionary, "OBCApp.epsBridge.EPS_GET_STATUS")

    def start(self) -> None:
        clean_dir(self.probe_root)
        clean_dir(self.runtime_root)
        SECURITY_SERVER_SOCKET.parent.mkdir(parents=True, exist_ok=True)
        self.processes.append(
            start_managed_process(
                "security_server_sim",
                [
                    PYTHON_BIN,
                    str(ROOT_DIR / "scripts/security_server_sim.py"),
                    "--socket-path",
                    str(SECURITY_SERVER_SOCKET),
                ],
                SECURITY_SERVER_LOG,
                cwd=ROOT_DIR,
                stale_match_groups=((str(SECURITY_SERVER_SOCKET),),),
                stale_match_markers=("security_server_sim.py",),
            )
        )
        self.wait_for_socket(SECURITY_SERVER_SOCKET, HOSTED_TIMEOUT_SEC)
        self.stack = HostedPerBandStockStacks(
            mode_name="combined",
            root_dir=ROOT_DIR,
            bin_dir=BIN_DIR,
            dictionary_path=DICT_PATH,
            cli_path=find_fprime_cli(ROOT_DIR),
            stack_root=self.stack_root,
            runtime_root=self.runtime_root,
            expose_sband_surface=True,
            expose_uhf_surface=True,
            preserve_sband_primary=True,
            enable_uhf_beacon_side_channel=True,
            auto_ports=True,
            command_authority_profile="sband-primary",
        )
        self.stack.start()
        if self.stack.sband is None or self.stack.uhf is None:
            raise RuntimeError("combined hosted stack did not expose both S-band and UHF surfaces")
        self.sband = self.stack.sband
        self.uhf = self.stack.uhf
        self.runtime_root = self.stack.runtime_root
        self.obc_log = self.stack.obc_log
        wait_text_since(self.obc_log, 0, "BEACON_PACKET_EMITTED", HOSTED_TIMEOUT_SEC)
        time.sleep(1.0)

    def stop(self) -> None:
        if self.stack is not None:
            self.stack.stop()
            self.stack = None
        cleanup_managed_processes(self.processes, timeout_sec=5.0)

    @staticmethod
    def wait_for_socket(socket_path: pathlib.Path, timeout: float) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if socket_path.exists():
                return
            time.sleep(0.1)
        raise RuntimeError(f"timed out waiting for socket {socket_path}")

    def capture_path(self, ground: GroundPath, direction: str) -> pathlib.Path:
        assert self.stack is not None
        return self.stack.captures_root / f"{ground.band}-{direction}.bin"

    def send_logged_raw_packet(self, ground: GroundPath, label: str, payload: bytes) -> None:
        with ground.raw_command_log.open("a", encoding="utf-8") as handle:
            handle.write(f"{label}: tts=127.0.0.1:{ground.gds_tts_port} bytes={len(payload)} payload={payload.hex()}\n")
        send_tts_raw_packet(ground.gds_tts_port, payload)

    def fresh_handshake_count(self, ground: GroundPath, service_id: int, message_type: HandshakeMessageType) -> int:
        return len(
            load_handshake_messages(
                self.capture_path(ground, "southbound-to-gds"),
                scid=ground.scid,
                vcid=ground.vcid,
                frame_size=ground.frame_size,
                service_id=service_id,
                message_type=message_type,
            )
        )

    def wait_for_handshake(self, ground: GroundPath, service_id: int, message_type: HandshakeMessageType, seen_count: int, *, timeout: float = AUTH_TIMEOUT_SEC):
        return wait_for_handshake_message(
            self.capture_path(ground, "southbound-to-gds"),
            scid=ground.scid,
            vcid=ground.vcid,
            frame_size=ground.frame_size,
            service_id=service_id,
            message_type=message_type,
            seen_count=seen_count,
            timeout=timeout,
        )

    def authenticate_service(self, ground: GroundPath, *, service_id: int, ingress_port: int, expected_open_fragment: str) -> SecureSession:
        challenge_count = self.fresh_handshake_count(ground, service_id, HandshakeMessageType.CHALLENGE)
        status_count = self.fresh_handshake_count(ground, service_id, HandshakeMessageType.AUTH_STATUS)
        deadline = time.time() + AUTH_ESTABLISH_TIMEOUT_SEC
        attempt = 0
        while time.time() < deadline:
            attempt += 1
            auth_offset = file_size(self.obc_log)
            self.send_logged_raw_packet(ground, f"{ground.name}-req-auth-service-{service_id}-attempt-{attempt}", build_req_auth_packet(service_id))
            try:
                challenge = self.wait_for_handshake(
                    ground,
                    service_id,
                    HandshakeMessageType.CHALLENGE,
                    challenge_count,
                    timeout=AUTH_TIMEOUT_SEC,
                )
            except TimeoutError:
                time.sleep(AUTH_RETRY_BACKOFF_SEC)
                continue
            challenge_count += 1
            session_key = request_session_key(SECURITY_SERVER_SOCKET, service_id, challenge.challenge)
            response = compute_auth_response(session_key)
            self.send_logged_raw_packet(
                ground,
                f"{ground.name}-auth-response-service-{service_id}-attempt-{attempt}",
                build_response_packet(service_id, response),
            )
            auth_status = self.wait_for_handshake(
                ground,
                service_id,
                HandshakeMessageType.AUTH_STATUS,
                status_count,
                timeout=AUTH_TIMEOUT_SEC,
            )
            status_count += 1
            if auth_status.status_code != AuthStatusCode.AUTHENTICATED:
                time.sleep(AUTH_RETRY_BACKOFF_SEC)
                continue
            wait_text_since(
                self.obc_log,
                auth_offset,
                f"Secure auth established ingress {ingress_port} service {service_id}",
                AUTH_TIMEOUT_SEC,
            )
            wait_regex_since(
                self.obc_log,
                auth_offset,
                re.escape(expected_open_fragment) + r".*session \d+",
                AUTH_TIMEOUT_SEC,
            )
            return SecureSession(service_id=service_id, session_key=session_key)
        raise RuntimeError(f"{ground.name}: failed to authenticate service {service_id}")

    def send_secure_command(
        self,
        ground: GroundPath,
        session: SecureSession,
        opcode: int,
        *,
        args: bytes = b"",
        log_waits: list[tuple[pathlib.Path, str]] = (),
        regex_waits: list[tuple[pathlib.Path, str]] = (),
        timeout: float = AUTH_TIMEOUT_SEC,
    ) -> int:
        offsets = {path: file_size(path) for path, _ in [*log_waits, *regex_waits]}
        sequence_number = session.accept_sequence()
        payload = build_secure_command_v2_packet(
            build_inner_command(opcode, args=args),
            session.session_key,
            sequence_number,
        )
        self.send_logged_raw_packet(ground, f"{ground.name}-secure-command-seq-{sequence_number}", payload)
        for path, fragment in log_waits:
            wait_text_since(path, offsets[path], fragment, timeout)
        for path, pattern in regex_waits:
            wait_regex_since(path, offsets[path], pattern, timeout)
        return sequence_number

    def wait_channel_search_hit(
        self,
        ground: GroundPath,
        label: str,
        search: str,
        timeout: float,
        *,
        log_offset: int | None = None,
    ) -> str:
        deadline = time.time() + timeout
        if log_offset is None:
            log_offset = file_size(ground.channels_log)
        last_output = ""
        while time.time() < deadline:
            output = ground.run_channel_capture(label, search)
            combined = read_text_since(ground.channels_log, log_offset)
            last_output = combined or output
            combined_hit = find_channel_search_hit(combined, search)
            if combined_hit is not None:
                return combined_hit
            output_hit = find_channel_search_hit(output, search)
            if output_hit is not None:
                return output_hit
            time.sleep(0.5)
        raise RuntimeError(
            f"{ground.name}: timed out waiting for bounded channel capture {search!r}; last={last_output}"
        )

    def prove_detailed_readback(
        self,
        session: SecureSession,
        *,
        label: str,
        opcode: int,
        event_fragment: str,
        fragment: str,
        timeout: float = 20.0,
    ) -> None:
        assert self.sband is not None
        channel_offset = file_size(self.sband.channels_log)
        self.send_secure_command(
            self.sband,
            session,
            opcode,
            log_waits=[(self.sband.events_log, event_fragment)],
            timeout=timeout,
        )
        hit_text = self.wait_channel_search_hit(
            self.sband,
            label,
            fragment,
            timeout,
            log_offset=channel_offset,
        )
        if fragment not in hit_text:
            raise RuntimeError(f"{self.sband.name}: bounded readback did not contain {fragment!r}")
        # Allow the one-shot detailed sample to drain through the passive
        # listener, then prove it does not continue as ambient live chatter.
        time.sleep(1.0)
        quiet_offset = file_size(self.sband.channels_log)
        quiet_deadline = time.time() + 2.5
        while time.time() < quiet_deadline:
            if fragment in read_text_since(self.sband.channels_log, quiet_offset):
                raise RuntimeError(f"{self.sband.name}: bounded readback fragment {fragment!r} continued after the hit")
            time.sleep(0.2)

    def assert_pre_auth_quiet(self) -> None:
        assert self.sband is not None
        event_offset = file_size(self.sband.events_log)
        channel_offset = file_size(self.sband.channels_log)
        capture_path = self.capture_path(self.sband, "southbound-to-gds")
        capture_size = file_size(capture_path)
        time.sleep(QUIET_WINDOW_SEC)
        if read_text_since(self.sband.events_log, event_offset).strip():
            raise RuntimeError("unexpected pre-auth S-band event visibility")
        if read_text_since(self.sband.channels_log, channel_offset).strip():
            raise RuntimeError("unexpected pre-auth S-band channel visibility")
        if file_size(capture_path) != capture_size:
            raise RuntimeError("unexpected pre-auth S-band downlink capture growth")
        self.summary.append("case-pre-auth-sband-live-quiet=PASS")

    @staticmethod
    def assert_fragments_absent(text: str, fragments: tuple[str, ...], label: str) -> None:
        found = [fragment for fragment in fragments if fragment in text]
        if found:
            raise RuntimeError(f"{label}: unexpected fragments present: {found}")

    def assert_sband_passive_logs_stable(self, quiet_sec: float = 2.5, timeout: float = 12.0) -> None:
        assert self.sband is not None
        wait_stable_size(self.sband.events_log, file_size(self.sband.events_log), quiet_sec=quiet_sec, timeout=timeout)
        if self.sband.channels_process is not None and self.sband.channels_process.process.poll() is None:
            wait_stable_size(
                self.sband.channels_log,
                file_size(self.sband.channels_log),
                quiet_sec=quiet_sec,
                timeout=timeout,
            )

    def wait_obc_runtime_marker_since(self, offset: int, pattern: str, timeout: float) -> tuple[int, int]:
        compiled = re.compile(pattern)
        deadline = time.time() + timeout
        while time.time() < deadline:
            for raw_line in read_text_since(self.obc_log, offset).splitlines():
                if compiled.search(raw_line) is None:
                    continue
                marker = parse_obc_runtime_marker(raw_line)
                if marker is not None:
                    return marker
            time.sleep(0.2)
        raise RuntimeError(f"timed out waiting for runtime marker {pattern!r} in {self.obc_log}")

    def live_capture_apid_counts(self, path: pathlib.Path) -> Counter[int]:
        data = path.read_bytes() if path.exists() else b""
        frames = parse_tm_frames(data, self.sband.scid, self.sband.vcid, self.sband.frame_size)  # type: ignore[union-attr]
        summary = packet_summary(frames)
        apid_counts = Counter({int(apid): int(count) for apid, count in summary["apid_counts"].items()})
        return Counter({apid: apid_counts.get(apid, 0) for apid in LIVE_DOWNLINK_APIDS})

    def wait_live_capture_quiet(
        self,
        path: pathlib.Path,
        baseline_counts: Counter[int],
        *,
        quiet_sec: float,
        timeout: float,
    ) -> None:
        deadline = time.time() + timeout
        quiet_start = time.time()
        last_counts = baseline_counts.copy()
        while time.time() < deadline:
            current_counts = self.live_capture_apid_counts(path)
            if current_counts != last_counts:
                last_counts = current_counts
                quiet_start = time.time()
                time.sleep(0.2)
                continue
            if (time.time() - quiet_start) >= quiet_sec:
                return
            time.sleep(0.2)
        raise RuntimeError(
            f"timed out waiting for live downlink APIDs to go quiet on {path}; baseline={dict(baseline_counts)} current={dict(last_counts)}"
        )

    def assert_no_post_close_visibility(
        self,
        *,
        event_offset: int,
        channel_offset: int,
        close_marker: tuple[int, int],
        settle_sec: float,
    ) -> None:
        assert self.sband is not None
        deadline = time.time() + settle_sec
        while time.time() < deadline:
            for path, offset in (
                (self.sband.events_log, event_offset),
                (self.sband.channels_log, channel_offset),
            ):
                for raw_line in read_text_since(path, offset).splitlines():
                    marker = parse_ground_runtime_marker(raw_line)
                    if marker is not None and marker > close_marker:
                        raise RuntimeError(f"post-close S-band visibility persisted in {path}: {raw_line}")
            time.sleep(0.2)

    def run(self) -> list[str]:
        assert self.sband is not None
        capture_path = self.capture_path(self.sband, "southbound-to-gds")

        self.assert_pre_auth_quiet()

        event_offset = file_size(self.sband.events_log)
        channel_offset = file_size(self.sband.channels_log)
        session = self.authenticate_service(
            self.sband,
            service_id=SERVICE_ID_SBAND,
            ingress_port=0,
            expected_open_fragment="Command session opened ingress 0 identity 1 role 1",
        )
        wait_text_since(
            self.sband.events_log,
            event_offset,
            "COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED",
            15.0,
        )
        self.summary.append("case-post-auth-sband-live-open=PASS")

        for fragment in (
            "OBCApp.epsBridge.EPS_SOC",
            "OBCApp.epsBridge.EPS_IBAT",
            "OBCApp.adcsBridge.ADCS_Q0",
            "OBCApp.adcsBridge.ADCS_OMEGA_X",
            "OBCApp.watchdogSupervisor.SYS_CPU_USAGE",
            "OBCApp.watchdogSupervisor.SYS_MEM_RSS_MB",
            # AVAILABLE is `update on change` and may be correctly suppressed
            # when authentication opens visibility without changing link
            # health. The age channel is periodic and is the deterministic
            # proof that the health-provider surface is reviewable.
            "OBCApp.groundLinkHealthProvider.GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS",
            "OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES",
            "OBCApp.commEgressMux.SBAND_ROUTED_EVENT_PACKETS",
        ):
            wait_text_since(self.sband.channels_log, channel_offset, fragment, 15.0)
        passive_live = read_text_since(self.sband.channels_log, channel_offset)
        self.assert_fragments_absent(
            passive_live,
            (
                "OBCApp.epsBridge.EPS_POWER_OUT",
                "OBCApp.gpsBridge.GPS_LAT_DEG",
                "OBCApp.adcsBridge.ADCS_MAG_X",
                "OBCApp.adcsBridge.ADCS_POINTING_ERR",
                "OBCApp.radioController.RADIO_RSSI",
                "OBCApp.storageHealthBridge.STORAGE_SCAN_COUNT",
            ),
            "post-auth curated live",
        )
        self.summary.append("case-post-auth-summary-live-curated=PASS")
        self.summary.append("case-post-auth-resource-reviewable-surfaces=PASS")

        self.prove_detailed_readback(
            session,
            label="eps-power-out-bounded-readback",
            opcode=self.opcode_eps_get_status,
            event_fragment="EPS_STATUS_RECEIVED",
            fragment="OBCApp.epsBridge.EPS_POWER_OUT",
        )
        self.summary.append("case-eps-get-bounded-detailed-readback=PASS")

        self.send_secure_command(
            self.sband,
            session,
            self.opcode_get_reset_cause,
            log_waits=[(self.sband.events_log, "BOOT_RECOVERY_STATUS")],
            timeout=20.0,
        )
        self.summary.append("case-authenticated-get-reset-cause-summary-readback=PASS")

        switch_obc_offset = file_size(self.obc_log)
        switch_event_offset = file_size(self.sband.events_log)
        switch_channel_offset = file_size(self.sband.channels_log)
        switch_capture_counts = self.live_capture_apid_counts(capture_path)
        self.send_secure_command(
            self.sband,
            session,
            self.opcode_comm_set_active,
            args=struct.pack(">B", COMM_BAND_UHF),
            log_waits=[
                (self.obc_log, "COMM_PRIMARY_LINK_CHANGED"),
                (self.obc_log, "Secure auth revoked ingress 0 service 1"),
                (self.obc_log, "Command session revoked ingress 0 identity 1 role 1"),
                (self.obc_log, "COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED"),
            ],
            regex_waits=[
                (self.obc_log, r"COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED.*active 0.*reason 3"),
            ],
            timeout=20.0,
        )
        close_marker = self.wait_obc_runtime_marker_since(
            switch_obc_offset,
            r"COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED.*active 0.*reason 3",
            5.0,
        )
        self.wait_live_capture_quiet(
            capture_path,
            switch_capture_counts,
            quiet_sec=5.0,
            timeout=10.0,
        )
        self.assert_no_post_close_visibility(
            event_offset=switch_event_offset,
            channel_offset=switch_channel_offset,
            close_marker=close_marker,
            settle_sec=5.0,
        )
        self.summary.append("case-primary-switch-closes-sband-live-observability=PASS")

        return [
            *self.summary,
            f"sband-events-log={self.sband.events_log}",
            f"sband-channels-log={self.sband.channels_log}",
            f"obc-log={self.obc_log}",
            f"sband-southbound-capture={capture_path}",
            f"runtime-root={self.runtime_root}",
            "hosted-sband-observability-governance=PASS",
        ]


def main() -> int:
    probe = HostedObservabilityGovernanceProbe()
    try:
        probe.start()
        lines = [
            "sband-observability-governance-hosted-probe: PASS",
            f"probe-root={PROBE_TMP_DIR}",
        ]
        lines.extend(probe.run())
        print("\n".join(lines))
        return 0
    finally:
        probe.stop()


if __name__ == "__main__":
    raise SystemExit(main())
