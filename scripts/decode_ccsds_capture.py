#!/usr/bin/env python3
"""Decode the bounded CCSDS capture artifacts produced by ground_ttc_gateway."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
from collections import Counter, defaultdict
from typing import Any


SPACE_PACKET_HEADER_SIZE = 6
TC_HEADER_SIZE = 5
TC_TRAILER_SIZE = 2
TM_HEADER_SIZE = 6
TM_TRAILER_SIZE = 2
IDLE_APID = 0x7FF
CRC16_CCITT_POLY = 0x1021
CRC16_CCITT_INIT = 0xFFFF
TC_FILL_PATTERN = b"sitting well"


def u16be(data: bytes, offset: int) -> int:
    return (data[offset] << 8) | data[offset + 1]


def crc16_ccitt(data: bytes) -> int:
    value = CRC16_CCITT_INIT
    for byte in data:
        value ^= byte << 8
        for _ in range(8):
            if value & 0x8000:
                value = ((value << 1) ^ CRC16_CCITT_POLY) & 0xFFFF
            else:
                value = (value << 1) & 0xFFFF
    return value


def parse_space_packets(payload: bytes, *, stop_at_idle: bool = False) -> list[dict[str, Any]]:
    packets: list[dict[str, Any]] = []
    offset = 0
    while offset + SPACE_PACKET_HEADER_SIZE <= len(payload):
        packet_id = u16be(payload, offset)
        sequence_control = u16be(payload, offset + 2)
        data_length_token = u16be(payload, offset + 4)
        apid = packet_id & 0x07FF
        sequence_flags = (sequence_control >> 14) & 0x03
        sequence_count = sequence_control & 0x3FFF
        total_size = SPACE_PACKET_HEADER_SIZE + data_length_token + 1
        if offset + total_size > len(payload):
            break
        packet = {
            "apid": apid,
            "sequence_flags": sequence_flags,
            "sequence_count": sequence_count,
            "offset": offset,
            "bytes": total_size,
            "idle": apid == IDLE_APID,
        }
        packets.append(packet)
        offset += total_size
        if stop_at_idle and apid == IDLE_APID:
            break
    return packets


def parse_tc_frames(data: bytes, expected_scid: int, expected_vcid: int) -> list[dict[str, Any]]:
    frames: list[dict[str, Any]] = []
    offset = 0
    while offset + TC_HEADER_SIZE + TC_TRAILER_SIZE <= len(data):
        flags_and_scid = u16be(data, offset)
        vcid_and_length = u16be(data, offset + 2)
        frame_length = (vcid_and_length & 0x03FF) + 1
        scid = flags_and_scid & 0x03FF
        vcid = (vcid_and_length >> 10) & 0x3F
        if (
            scid != expected_scid
            or vcid != expected_vcid
            or frame_length < TC_HEADER_SIZE + TC_TRAILER_SIZE
            or offset + frame_length > len(data)
        ):
            offset += 1
            continue
        frame = data[offset : offset + frame_length]
        transmitted_crc = u16be(frame, frame_length - TC_TRAILER_SIZE)
        computed_crc = crc16_ccitt(frame[: frame_length - TC_TRAILER_SIZE])
        if transmitted_crc != computed_crc:
            offset += 1
            continue
        data_field = data[offset + TC_HEADER_SIZE : offset + frame_length - TC_TRAILER_SIZE]
        packets = parse_space_packets(data_field)
        frames.append(
            {
                "offset": offset,
                "bytes": frame_length,
                "scid": scid,
                "vcid": vcid,
                "frame_sequence_count": data[offset + 4],
                "scid_ok": scid == expected_scid,
                "vcid_ok": vcid == expected_vcid,
                "crc_ok": True,
                "packets": packets,
            }
        )
        offset += frame_length
    return frames


def parse_tm_frames(data: bytes, expected_scid: int, expected_vcid: int, frame_size: int) -> list[dict[str, Any]]:
    frames: list[dict[str, Any]] = []
    offset = 0
    while offset + frame_size <= len(data):
        frame = data[offset : offset + frame_size]
        global_vcid = u16be(frame, 0)
        scid = (global_vcid >> 4) & 0x03FF
        vcid = (global_vcid >> 1) & 0x07
        data_field = frame[TM_HEADER_SIZE : frame_size - TM_TRAILER_SIZE]
        transmitted_crc = u16be(frame, frame_size - TM_TRAILER_SIZE)
        computed_crc = crc16_ccitt(frame[: frame_size - TM_TRAILER_SIZE])
        if scid != expected_scid or vcid != expected_vcid or transmitted_crc != computed_crc:
            offset += 1
            continue
        frames.append(
            {
                "offset": offset,
                "bytes": frame_size,
                "scid": scid,
                "vcid": vcid,
                "master_frame_count": frame[2],
                "virtual_frame_count": frame[3],
                "data_field_status": u16be(frame, 4),
                "scid_ok": True,
                "vcid_ok": True,
                "crc_ok": True,
                "packets": parse_space_packets(data_field, stop_at_idle=True),
            }
        )
        offset += frame_size
    return frames


def tm_non_frame_segments(data: bytes, frames: list[dict[str, Any]]) -> list[dict[str, Any]]:
    segments: list[dict[str, Any]] = []
    previous_end = 0
    for frame in sorted(frames, key=lambda item: int(item["offset"])):
        offset = int(frame["offset"])
        if offset > previous_end:
            segment = data[previous_end:offset]
            segments.append(
                {
                    "offset": previous_end,
                    "bytes": len(segment),
                    "fill_ok": is_repeated_pattern(segment, TC_FILL_PATTERN),
                    "at_end": False,
                }
            )
        previous_end = offset + int(frame["bytes"])
    if previous_end < len(data):
        segment = data[previous_end:]
        segments.append(
            {
                "offset": previous_end,
                "bytes": len(segment),
                "fill_ok": is_repeated_pattern(segment, TC_FILL_PATTERN),
                "at_end": True,
            }
        )
    return segments


def packet_summary(frames: list[dict[str, Any]]) -> dict[str, Any]:
    apid_counts: Counter[int] = Counter()
    sequence_counts: dict[int, list[int]] = defaultdict(list)
    packet_count = 0
    for frame in frames:
        for packet in frame["packets"]:
            if packet["idle"]:
                continue
            apid = int(packet["apid"])
            apid_counts[apid] += 1
            sequence_counts[apid].append(int(packet["sequence_count"]))
            packet_count += 1
    return {
        "frames": len(frames),
        "packets": packet_count,
        "apid_counts": {str(apid): count for apid, count in sorted(apid_counts.items())},
        "sequence_counts": {str(apid): values for apid, values in sorted(sequence_counts.items())},
        "frame_offsets": [int(frame["offset"]) for frame in frames],
    }


def is_repeated_pattern(data: bytes, pattern: bytes) -> bool:
    return len(data) % len(pattern) == 0 and data == pattern * (len(data) // len(pattern))


def tc_non_frame_segments(data: bytes, frames: list[dict[str, Any]]) -> list[dict[str, Any]]:
    segments: list[dict[str, Any]] = []
    previous_end = 0
    for frame in sorted(frames, key=lambda item: int(item["offset"])):
        offset = int(frame["offset"])
        if offset > previous_end:
            segment = data[previous_end:offset]
            segments.append(
                {
                    "offset": previous_end,
                    "bytes": len(segment),
                    "fill_ok": is_repeated_pattern(segment, TC_FILL_PATTERN),
                }
            )
        previous_end = offset + int(frame["bytes"])
    if previous_end < len(data):
        segment = data[previous_end:]
        segments.append(
            {
                "offset": previous_end,
                "bytes": len(segment),
                "fill_ok": is_repeated_pattern(segment, TC_FILL_PATTERN),
            }
        )
    return segments


def require_apids(summary: dict[str, Any], required: list[int], direction: str) -> list[str]:
    counts = summary["apid_counts"]
    missing = [apid for apid in required if counts.get(str(apid), 0) <= 0]
    if not missing:
        return []
    return [f"{direction} missing required APID(s): {', '.join(str(apid) for apid in missing)}"]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--uplink", type=pathlib.Path, required=True)
    parser.add_argument("--downlink", type=pathlib.Path, required=True)
    parser.add_argument("--scid", type=lambda value: int(value, 0), default=0x44)
    parser.add_argument("--vcid", type=lambda value: int(value, 0), default=1)
    parser.add_argument("--tm-frame-size", type=int, default=1024)
    parser.add_argument("--require-uplink-apid", type=lambda value: int(value, 0), action="append", default=[])
    parser.add_argument("--require-downlink-apid", type=lambda value: int(value, 0), action="append", default=[])
    parser.add_argument("--summary", type=pathlib.Path)
    parser.add_argument("--include-frames", action="store_true")
    parser.add_argument("--allow-downlink-trailing-partial", action="store_true")
    args = parser.parse_args()

    uplink_data = args.uplink.read_bytes()
    downlink_data = args.downlink.read_bytes()
    uplink_frames = parse_tc_frames(uplink_data, args.scid, args.vcid)
    downlink_frames = parse_tm_frames(downlink_data, args.scid, args.vcid, args.tm_frame_size)
    uplink_non_frame_segments = tc_non_frame_segments(uplink_data, uplink_frames)
    downlink_non_frame_segments = tm_non_frame_segments(downlink_data, downlink_frames)
    uplink_summary = packet_summary(uplink_frames)
    downlink_summary = packet_summary(downlink_frames)
    uplink_summary["fill_bytes"] = sum(int(segment["bytes"]) for segment in uplink_non_frame_segments if segment["fill_ok"])
    uplink_summary["unexpected_non_frame_bytes"] = sum(
        int(segment["bytes"]) for segment in uplink_non_frame_segments if not segment["fill_ok"]
    )
    uplink_summary["non_frame_segments"] = uplink_non_frame_segments
    downlink_summary["fill_bytes"] = sum(int(segment["bytes"]) for segment in downlink_non_frame_segments if segment["fill_ok"])
    downlink_summary["non_frame_segments"] = downlink_non_frame_segments
    trailing_partial_bytes = sum(
        int(segment["bytes"])
        for segment in downlink_non_frame_segments
        if segment["at_end"]
    )
    downlink_summary["trailing_bytes"] = trailing_partial_bytes
    downlink_summary["unexpected_non_frame_bytes"] = sum(
        int(segment["bytes"])
        for segment in downlink_non_frame_segments
        if not segment["fill_ok"] and not segment["at_end"]
    )
    summary = {
        "scid": args.scid,
        "vcid": args.vcid,
        "tm_frame_size": args.tm_frame_size,
        "uplink": uplink_summary,
        "downlink": downlink_summary,
    }
    if args.include_frames:
        summary["uplink_frames"] = uplink_frames
        summary["downlink_frames"] = downlink_frames

    errors = []
    if any(not frame["scid_ok"] or not frame["vcid_ok"] or not frame["crc_ok"] for frame in uplink_frames):
        errors.append("uplink contains TC frame(s) with unexpected SCID/VCID/FECF")
    if any(not frame["scid_ok"] or not frame["vcid_ok"] or not frame["crc_ok"] for frame in downlink_frames):
        errors.append("downlink contains TM frame(s) with unexpected SCID/VCID/FECF")
    if any(not segment["fill_ok"] for segment in uplink_non_frame_segments):
        errors.append("uplink capture has unexpected non-frame byte(s) outside the known TC fill pattern")
    if downlink_summary["unexpected_non_frame_bytes"]:
        errors.append("downlink capture has unexpected non-frame byte(s) before the last complete TM frame")
    if trailing_partial_bytes:
        if trailing_partial_bytes >= args.tm_frame_size:
            errors.append(
                "downlink capture has unexpected non-frame byte(s) spanning at least one full TM frame after complete TM frames"
            )
        elif not args.allow_downlink_trailing_partial:
            errors.append(f"downlink capture has {trailing_partial_bytes} trailing byte(s) after complete TM frames")
    errors.extend(require_apids(uplink_summary, args.require_uplink_apid, "uplink"))
    errors.extend(require_apids(downlink_summary, args.require_downlink_apid, "downlink"))
    if not uplink_frames:
        errors.append("uplink capture contains no complete TC frames")
    if not downlink_frames:
        errors.append("downlink capture contains no complete TM frames")

    rendered = json.dumps(summary, indent=2, sort_keys=True)
    if args.summary:
        args.summary.write_text(rendered + "\n", encoding="utf-8")
    print(rendered)
    if errors:
        for error in errors:
            print(f"ccsds-capture-decode: ERROR: {error}", file=sys.stderr)
        return 1
    print("ccsds-capture-decode: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
