#!/usr/bin/env python3
"""Unit test for scripts/decode_ccsds_capture.py."""

from __future__ import annotations

import importlib.util
import pathlib
import tempfile


SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("decode_ccsds_capture", SCRIPT_DIR / "decode_ccsds_capture.py")
assert SPEC is not None and SPEC.loader is not None
decoder = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(decoder)


def space_packet(apid: int, sequence_count: int, payload: bytes) -> bytes:
    packet_id = apid & 0x07FF
    sequence_control = (0x3 << 14) | (sequence_count & 0x3FFF)
    data_length = len(payload) - 1
    return (
        packet_id.to_bytes(2, "big")
        + sequence_control.to_bytes(2, "big")
        + data_length.to_bytes(2, "big")
        + payload
    )


def tc_frame(scid: int, vcid: int, sequence: int, data: bytes) -> bytes:
    frame_length = decoder.TC_HEADER_SIZE + len(data) + decoder.TC_TRAILER_SIZE
    length_token = frame_length - 1
    frame_no_crc = (
        (scid & 0x03FF).to_bytes(2, "big")
        + (((vcid & 0x3F) << 10) | (length_token & 0x03FF)).to_bytes(2, "big")
        + bytes([sequence])
        + data
    )
    return frame_no_crc + decoder.crc16_ccitt(frame_no_crc).to_bytes(2, "big")


def tm_frame(scid: int, vcid: int, master_count: int, data_packets: bytes, frame_size: int) -> bytes:
    global_vcid = ((scid & 0x03FF) << 4) | ((vcid & 0x07) << 1)
    idle_payload_size = frame_size - decoder.TM_HEADER_SIZE - decoder.TM_TRAILER_SIZE - len(data_packets)
    idle = space_packet(0x7FF, 0, b"\xE0" * (idle_payload_size - decoder.SPACE_PACKET_HEADER_SIZE))
    body = data_packets + idle
    assert len(body) == frame_size - decoder.TM_HEADER_SIZE - decoder.TM_TRAILER_SIZE
    frame_no_crc = (
        global_vcid.to_bytes(2, "big")
        + bytes([master_count, master_count])
        + (0x1800).to_bytes(2, "big")
        + body
    )
    return frame_no_crc + decoder.crc16_ccitt(frame_no_crc).to_bytes(2, "big")


def main() -> int:
    scid = 0x44
    vcid = 1
    frame_size = 96
    uplink_frame = tc_frame(scid, vcid, 7, space_packet(0, 3, b"command"))
    uplink = b"sitting well" + uplink_frame + b"sitting wellsitting well"
    noisy_uplink = b"not-frame" + uplink
    downlink_payload = (
        tm_frame(scid, vcid, 1, space_packet(1, 4, b"tlm"), frame_size)
        + tm_frame(scid, vcid, 2, space_packet(2, 5, b"event") + space_packet(3, 6, b"file"), frame_size)
    )
    downlink = b"sitting well" + downlink_payload
    downlink_with_partial = downlink + downlink_payload[:17]
    downlink_with_corrupt_full_tail = downlink + (b"\xAA" * frame_size)
    downlink_with_fill_pattern_full_tail = downlink + (decoder.TC_FILL_PATTERN * (frame_size // len(decoder.TC_FILL_PATTERN)))

    uplink_frames = decoder.parse_tc_frames(uplink, scid, vcid)
    downlink_frames = decoder.parse_tm_frames(downlink, scid, vcid, frame_size)
    assert decoder.packet_summary(uplink_frames)["apid_counts"] == {"0": 1}
    assert decoder.packet_summary(uplink_frames)["frame_offsets"] == [12]
    segments = decoder.tc_non_frame_segments(uplink, uplink_frames)
    assert sum(segment["bytes"] for segment in segments) == 36
    assert all(segment["fill_ok"] for segment in segments)
    assert decoder.packet_summary(downlink_frames)["apid_counts"] == {"1": 1, "2": 1, "3": 1}
    assert decoder.packet_summary(downlink_frames)["frame_offsets"] == [12, 108]
    assert all(frame["crc_ok"] for frame in downlink_frames)
    tm_segments = decoder.tm_non_frame_segments(downlink, downlink_frames)
    assert tm_segments == [{"offset": 0, "bytes": 12, "fill_ok": True, "at_end": False}]

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = pathlib.Path(tmp)
        uplink_path = tmp_path / "uplink.bin"
        downlink_path = tmp_path / "downlink.bin"
        summary_path = tmp_path / "summary.json"
        uplink_path.write_bytes(uplink)
        downlink_path.write_bytes(downlink)
        import subprocess
        import sys

        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "decode_ccsds_capture.py"),
                "--uplink",
                str(uplink_path),
                "--downlink",
                str(downlink_path),
                "--scid",
                str(scid),
                "--vcid",
                str(vcid),
                "--tm-frame-size",
                str(frame_size),
                "--require-uplink-apid",
                "0",
                "--require-downlink-apid",
                "1",
                "--require-downlink-apid",
                "2",
                "--require-downlink-apid",
                "3",
                "--summary",
                str(summary_path),
            ],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        assert result.returncode == 0, result.stderr
        assert summary_path.is_file()
        summary = summary_path.read_text(encoding="utf-8")
        assert '"fill_bytes": 36' in summary
        assert '"unexpected_non_frame_bytes": 0' in summary
        assert '"trailing_bytes": 0' in summary

        uplink_path.write_bytes(noisy_uplink)
        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "decode_ccsds_capture.py"),
                "--uplink",
                str(uplink_path),
                "--downlink",
                str(downlink_path),
                "--scid",
                str(scid),
                "--vcid",
                str(vcid),
                "--tm-frame-size",
                str(frame_size),
                "--require-uplink-apid",
                "0",
            ],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        assert result.returncode == 1
        assert "unexpected non-frame byte" in result.stderr

        uplink_path.write_bytes(uplink)
        downlink_path.write_bytes(downlink_with_partial)
        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "decode_ccsds_capture.py"),
                "--uplink",
                str(uplink_path),
                "--downlink",
                str(downlink_path),
                "--scid",
                str(scid),
                "--vcid",
                str(vcid),
                "--tm-frame-size",
                str(frame_size),
                "--allow-downlink-trailing-partial",
            ],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        assert result.returncode == 0, result.stderr
        assert '"trailing_bytes": 17' in result.stdout

        downlink_path.write_bytes(downlink_with_corrupt_full_tail)
        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "decode_ccsds_capture.py"),
                "--uplink",
                str(uplink_path),
                "--downlink",
                str(downlink_path),
                "--scid",
                str(scid),
                "--vcid",
                str(vcid),
                "--tm-frame-size",
                str(frame_size),
                "--allow-downlink-trailing-partial",
            ],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        assert result.returncode == 1
        assert "at least one full TM frame" in result.stderr

        downlink_path.write_bytes(downlink_with_fill_pattern_full_tail)
        result = subprocess.run(
            [
                sys.executable,
                str(SCRIPT_DIR / "decode_ccsds_capture.py"),
                "--uplink",
                str(uplink_path),
                "--downlink",
                str(downlink_path),
                "--scid",
                str(scid),
                "--vcid",
                str(vcid),
                "--tm-frame-size",
                str(frame_size),
                "--allow-downlink-trailing-partial",
            ],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        assert result.returncode == 1
        assert "at least one full TM frame" in result.stderr

    print("decode-ccsds-capture-unit-test: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
