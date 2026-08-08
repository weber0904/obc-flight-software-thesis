#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import pathlib
import struct
import sys
import zlib
from typing import Any


BEACON_MAGIC = 0x3143424F
BEACON_VERSION = 2
BEACON_V1_WIRE_SIZE = 108
BEACON_V1_FORMAT = "<IHHIIIIIBBHIIIIiiiiiBBBBIIIIIIIII"


class DecodeError(RuntimeError):
    pass


def decode_beacon_v1(payload: bytes) -> dict[str, Any]:
    if len(payload) != BEACON_V1_WIRE_SIZE:
        raise DecodeError(f"expected {BEACON_V1_WIRE_SIZE} bytes, got {len(payload)}")
    observed_crc = struct.unpack_from("<I", payload, len(payload) - 4)[0]
    expected_crc = zlib.crc32(payload[:-4]) & 0xFFFFFFFF
    if observed_crc != expected_crc:
        raise DecodeError(f"CRC mismatch: observed=0x{observed_crc:08x} expected=0x{expected_crc:08x}")

    unpacked = struct.unpack(BEACON_V1_FORMAT, payload)
    (
        magic,
        version,
        reserved,
        sequence,
        time_base,
        context,
        seconds,
        useconds,
        mode,
        boot_slot,
        reboot_count,
        uptime_sec,
        health_mask,
        fault_mask,
        quality_mask,
        battery_soc_x100,
        battery_voltage_x100,
        battery_current_x100,
        battery_temp_x100,
        adcs_rate_x1e6,
        adcs_mode,
        gps_fix_valid,
        storage_warning_mask,
        storage_degraded_mask,
        comm_pass_remaining_sec,
        comm_total_passes,
        gps_accepted_sentences,
        gps_rejected_sentences,
        csp_tx_packets,
        csp_rx_packets,
        radio_tx_bytes,
        radio_rx_bytes,
        decoded_crc,
    ) = unpacked
    if magic != BEACON_MAGIC:
        raise DecodeError(f"bad beacon magic: 0x{magic:08x}")
    if version != BEACON_VERSION:
        raise DecodeError(f"unsupported beacon version: {version}")
    if decoded_crc != observed_crc:
        raise DecodeError("internal CRC decode mismatch")

    return {
        "type": "BeaconV1",
        "version": version,
        "reserved": reserved,
        "sequence": sequence,
        "time": {
            "time_base": time_base,
            "context": context,
            "seconds": seconds,
            "useconds": useconds,
        },
        "mode": mode,
        "boot_slot": boot_slot,
        "reboot_count": reboot_count,
        "uptime_sec": uptime_sec,
        "health_mask": health_mask,
        "fault_mask": fault_mask,
        "quality_mask": quality_mask,
        "battery_soc": battery_soc_x100 / 100.0,
        "battery_voltage": battery_voltage_x100 / 100.0,
        "battery_current": battery_current_x100 / 100.0,
        "battery_temp_c": battery_temp_x100 / 100.0,
        "adcs_rate_norm": adcs_rate_x1e6 / 1_000_000.0,
        "adcs_mode": adcs_mode,
        "gps_fix_valid": gps_fix_valid != 0,
        "storage_warning_mask": storage_warning_mask,
        "storage_degraded_mask": storage_degraded_mask,
        "comm_pass_remaining_sec": comm_pass_remaining_sec,
        "comm_total_passes": comm_total_passes,
        "gps_accepted_sentences": gps_accepted_sentences,
        "gps_rejected_sentences": gps_rejected_sentences,
        "csp_tx_packets": csp_tx_packets,
        "csp_rx_packets": csp_rx_packets,
        "radio_tx_bytes": radio_tx_bytes,
        "radio_rx_bytes": radio_rx_bytes,
        "crc": observed_crc,
    }


def flatten(decoded: dict[str, Any]) -> dict[str, Any]:
    time = decoded["time"]
    return {
        "sequence": decoded["sequence"],
        "time_seconds": time["seconds"],
        "mode": decoded["mode"],
        "boot_slot": decoded["boot_slot"],
        "health_mask": decoded["health_mask"],
        "fault_mask": decoded["fault_mask"],
        "quality_mask": decoded["quality_mask"],
        "battery_soc": decoded["battery_soc"],
        "battery_voltage": decoded["battery_voltage"],
        "battery_current": decoded["battery_current"],
        "battery_temp_c": decoded["battery_temp_c"],
        "adcs_rate_norm": decoded["adcs_rate_norm"],
        "gps_fix_valid": decoded["gps_fix_valid"],
        "crc": decoded["crc"],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Decode a BeaconV1 wire-layout frame captured by hosted evidence tooling.")
    parser.add_argument("path", type=pathlib.Path, help="BeaconV1 wire-layout .bin path")
    parser.add_argument("--format", choices=("json", "csv"), default="json")
    args = parser.parse_args()

    try:
        decoded = decode_beacon_v1(args.path.read_bytes())
    except (OSError, DecodeError, struct.error) as error:
        print(f"decode_beacon_v1.py: {error}", file=sys.stderr)
        return 1

    if args.format == "json":
        print(json.dumps(decoded, indent=2, sort_keys=True))
    else:
        row = flatten(decoded)
        writer = csv.DictWriter(sys.stdout, fieldnames=list(row.keys()))
        writer.writeheader()
        writer.writerow(row)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
