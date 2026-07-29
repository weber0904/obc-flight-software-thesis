#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass


ROOT_DIR = pathlib.Path(__file__).resolve().parents[3]
COMPARATOR_SCRIPT = ROOT_DIR / "scripts" / "run_target_secure_auth_command_path_probe.sh"


def align16(value: int) -> int:
    return max(16, (value // 16) * 16)


def next_midpoint(lower_pass: int, upper_fail: int) -> int:
    candidate = align16((lower_pass + upper_fail) // 2)
    if candidate <= lower_pass:
        candidate = lower_pass + 16
    if candidate >= upper_fail:
        candidate = upper_fail - 16
    return candidate


@dataclass
class CaseResult:
    data_bytes: int
    window_frames: int
    interframe_delay_usec: int
    socketcan_tx_frame_delay_usec: int
    verdict: str
    failure_stage: str
    failure_error: str
    challenge_issued_count: int
    begin_total_frames: list[int]
    begin_total_bytes: list[int]
    node5_seen_frames: list[int]
    ack_results: list[str]
    contiguous_frames: list[int]
    drop_packet_lengths: list[int]
    probe_root: str
    summary_path: str


def run_case(
    probe_root: pathlib.Path,
    data_bytes: int,
    window_frames: int,
    interframe_delay_usec: int,
    socketcan_tx_frame_delay_usec: int,
) -> CaseResult:
    case_root = probe_root / (
        f"mtu{data_bytes}-w{window_frames}-d{interframe_delay_usec}-can{socketcan_tx_frame_delay_usec}"
    )
    if case_root.exists():
        shutil.rmtree(case_root)
    case_root.mkdir(parents=True, exist_ok=True)

    env = os.environ.copy()
    env["PROBE_ROOT"] = str(case_root)
    env["COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES"] = str(data_bytes)
    env["COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE"] = str(window_frames)
    env["COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC"] = str(interframe_delay_usec)
    env["COMM_CSP_SOCKETCAN_TX_FRAME_DELAY_USEC"] = str(socketcan_tx_frame_delay_usec)
    env.setdefault("OBC_GROUNDLINK_DIAGNOSTICS", "1")
    env.setdefault("SBAND_COMM_NODE_INGRESS_DIAGNOSTICS", "1")
    proc = subprocess.run(
        ["bash", str(COMPARATOR_SCRIPT)],
        cwd=str(ROOT_DIR),
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        check=False,
    )

    summary_path = case_root / "diagnostics" / "secure-auth-command-path-summary.json"
    summary_payload: dict[str, object] = {}
    if summary_path.exists():
        summary_payload = json.loads(summary_path.read_text(encoding="utf-8"))
    failure = summary_payload.get("failure", {}) if isinstance(summary_payload.get("failure", {}), dict) else {}
    failure_stage = str(failure.get("stage", ""))
    failure_error = str(failure.get("error", ""))

    obc_journal = case_root / "diagnostics" / "journal-snapshots" / "secure-auth-command-path-obc.log"
    sband_service_journal = case_root / "diagnostics" / "journal-snapshots" / "secure-auth-command-path-sband-service.log"
    obc_text = obc_journal.read_text(encoding="utf-8", errors="replace") if obc_journal.exists() else ""
    sband_text = sband_service_journal.read_text(encoding="utf-8", errors="replace") if sband_service_journal.exists() else ""

    begin_total_frames = [int(x) for x in re.findall(r"begin-ok .* total-frames=(\d+)", obc_text)]
    begin_total_bytes = [int(x) for x in re.findall(r"begin-ok .* total-bytes=(\d+)", obc_text)]
    node5_seen_frames = sorted({int(x) for x in re.findall(r"downlink-v3-data-stream=\d+ frame-index=(\d+)", sband_text)})
    ack_results = re.findall(r"downlink-v3-control-op=ACK_POLL .* result=([A-Z_]+)", sband_text)
    contiguous_frames = [int(x) for x in re.findall(r"reply-contiguous-frames=(\d+)", sband_text)]
    drop_packet_lengths = [int(x) for x in re.findall(r"downlink-v3-data-drop=1 packet-length=(\d+)", sband_text)]
    challenge_issued_count = obc_text.count("SECURE_AUTH_CHALLENGE_ISSUED")

    return CaseResult(
        data_bytes=data_bytes,
        window_frames=window_frames,
        interframe_delay_usec=interframe_delay_usec,
        socketcan_tx_frame_delay_usec=socketcan_tx_frame_delay_usec,
        verdict="PASS" if proc.returncode == 0 else "FAIL",
        failure_stage=failure_stage,
        failure_error=failure_error,
        challenge_issued_count=challenge_issued_count,
        begin_total_frames=begin_total_frames,
        begin_total_bytes=begin_total_bytes,
        node5_seen_frames=node5_seen_frames,
        ack_results=ack_results,
        contiguous_frames=contiguous_frames,
        drop_packet_lengths=drop_packet_lengths,
        probe_root=str(case_root),
        summary_path=str(summary_path),
    )


def classification_for_results(results: list[CaseResult]) -> str:
    if results[0].verdict == "PASS":
        return "pass-window3"
    if len(results) > 1 and results[1].verdict == "PASS":
        return "burst-limited-window1-pass"
    if len(results) > 2 and results[2].verdict == "PASS":
        return "burst-limited-pacing-pass"
    return "size-or-envelope-fail"


def case_payload(result: CaseResult) -> dict[str, object]:
    return {
        "dataBytes": result.data_bytes,
        "windowFrames": result.window_frames,
        "interframeDelayUsec": result.interframe_delay_usec,
        "socketcanTxFrameDelayUsec": result.socketcan_tx_frame_delay_usec,
        "verdict": result.verdict,
        "failureStage": result.failure_stage,
        "failureError": result.failure_error,
        "challengeIssuedCount": result.challenge_issued_count,
        "beginTotalFrames": result.begin_total_frames,
        "beginTotalBytes": result.begin_total_bytes,
        "node5SeenFrames": result.node5_seen_frames,
        "ackResults": result.ack_results,
        "contiguousFrames": result.contiguous_frames,
        "dropPacketLengths": result.drop_packet_lengths,
        "probeRoot": result.probe_root,
        "summaryPath": result.summary_path,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe-root", required=True)
    args = parser.parse_args()

    probe_root = pathlib.Path(args.probe_root)
    if probe_root.exists():
        shutil.rmtree(probe_root)
    probe_root.mkdir(parents=True, exist_ok=True)

    lower_pass = int(os.environ.get("TARGET_V3_SEARCH_LOWER_PASS_BYTES", "240"))
    upper_fail = int(os.environ.get("TARGET_V3_SEARCH_UPPER_FAIL_BYTES", "1000"))
    target_bytes = int(os.environ.get("TARGET_V3_SEARCH_TARGET_BYTES", "2032"))
    pacing_usec = int(os.environ.get("TARGET_V3_SEARCH_PACING_USEC", "500"))
    socketcan_tx_frame_delay_usec = int(os.environ.get("TARGET_V3_SEARCH_SOCKETCAN_TX_FRAME_DELAY_USEC", "0"))
    max_iterations = int(os.environ.get("TARGET_V3_SEARCH_MAX_ITERATIONS", "12"))

    if lower_pass <= 0 or upper_fail <= lower_pass:
        raise SystemExit("invalid adaptive-search anchor interval")

    history: list[dict[str, object]] = []
    tested_candidates: set[int] = set()
    start_ts = time.time()

    while (upper_fail - lower_pass) > 16 and len(tested_candidates) < max_iterations:
        candidate = next_midpoint(lower_pass, upper_fail)
        if candidate in tested_candidates:
            break
        tested_candidates.add(candidate)

        runs = [
            run_case(probe_root, candidate, 3, 0, socketcan_tx_frame_delay_usec),
            run_case(probe_root, candidate, 1, 0, socketcan_tx_frame_delay_usec),
            run_case(probe_root, candidate, 1, pacing_usec, socketcan_tx_frame_delay_usec),
        ]
        classification = classification_for_results(runs)
        history.append(
            {
                "candidateBytes": candidate,
                "classification": classification,
                "runs": [case_payload(run) for run in runs],
            }
        )
        if runs[0].verdict == "PASS":
            lower_pass = candidate
        else:
            upper_fail = candidate

    final_2032_runs = [
        run_case(probe_root, target_bytes, 3, 0, socketcan_tx_frame_delay_usec),
        run_case(probe_root, target_bytes, 1, 0, socketcan_tx_frame_delay_usec),
        run_case(probe_root, target_bytes, 1, pacing_usec, socketcan_tx_frame_delay_usec),
    ]
    final_2032_classification = classification_for_results(final_2032_runs)

    summary = {
        "mode": "node5-comm-csp-downlink-v3-target-adaptive-search",
        "startedAtUnixSec": start_ts,
        "finishedAtUnixSec": time.time(),
        "targetBytes": target_bytes,
        "pacingUsec": pacing_usec,
        "socketcanTxFrameDelayUsec": socketcan_tx_frame_delay_usec,
        "initialAnchors": {
            "lowerPassBytes": int(os.environ.get("TARGET_V3_SEARCH_LOWER_PASS_BYTES", "240")),
            "upperFailBytes": int(os.environ.get("TARGET_V3_SEARCH_UPPER_FAIL_BYTES", "1000")),
        },
        "finalInterval": {
            "lowerPassBytes": lower_pass,
            "upperFailBytes": upper_fail,
            "widthBytes": upper_fail - lower_pass,
        },
        "history": history,
        "target2032": {
            "classification": final_2032_classification,
            "runs": [case_payload(run) for run in final_2032_runs],
        },
    }
    summary_path = probe_root / "adaptive-search-summary.json"
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print("node5-comm-csp-downlink-v3-target-adaptive-search: PASS")
    print(f"probe-root={probe_root}")
    print(f"adaptive-search-summary={summary_path}")
    print(f"interval-lower-pass-bytes={lower_pass}")
    print(f"interval-upper-fail-bytes={upper_fail}")
    print(f"interval-width-bytes={upper_fail - lower_pass}")
    print(f"target-2032-classification={final_2032_classification}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
