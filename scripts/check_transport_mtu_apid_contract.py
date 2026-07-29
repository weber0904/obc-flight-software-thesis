#!/usr/bin/env python3
"""Validate the current transport ceiling and APID governance contract."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(relpath: str) -> str:
    return (ROOT / relpath).read_text(encoding="utf-8")


def expect(pattern: str, text: str, desc: str) -> re.Match[str]:
    match = re.search(pattern, text, re.MULTILINE)
    if not match:
        raise AssertionError(f"missing {desc}: {pattern}")
    return match


def expect_contains(snippet: str, text: str, desc: str) -> None:
    if snippet not in text:
        raise AssertionError(f"missing {desc}: {snippet}")


SIZEOF = {
    "U8": 1,
    "U16": 2,
    "U32": 4,
}


def evaluate_size_expr(expr: str, definitions: dict[str, int]) -> int:
    total = 0
    for term in [part.strip() for part in expr.split("+")]:
        sizeof_match = re.fullmatch(r"sizeof\((\w+)\)", term)
        if sizeof_match:
            token = sizeof_match.group(1)
            if token not in SIZEOF:
                raise AssertionError(f"unknown sizeof token: {token}")
            total += SIZEOF[token]
            continue
        if term not in definitions:
            raise AssertionError(f"unknown size definition token: {term}")
        total += definitions[term]
    return total


def parse_apid_enum(text: str) -> dict[str, str]:
    block_match = expect(
        r"dictionary enum Apid : FwPacketDescriptorType \{(?P<body>[\s\S]*?)\n\s*\} default",
        text,
        "ComCfg.Apid enum block",
    )
    body = block_match.group("body")
    result: dict[str, str] = {}
    for name, value in re.findall(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)", body, re.MULTILINE):
        result[name] = value.upper().replace("X", "x")
    return result


def main() -> int:
    project_fp = read("config/FpConstants.fpp")
    fp_config = read("lib/fprime/default/config/FpConfig.fpp")
    com_cfg = read("lib/fprime/default/config/ComCfg.fpp")
    com_ccsds_cfg = read("lib/fprime/Svc/Subtopologies/ComCcsds/ComCcsdsConfig/ComCcsdsConfig.fpp")
    obc_com_ccsds_cfg = read("OBC/TopCcsds/OBCComCcsdsConfig/OBCComCcsdsConfig.fpp")
    envelope = read("OBC/Components/CommandIngressAuthority/CommandEnvelopeMetadata.hpp")
    filepacket = read("lib/fprime/Fw/FilePacket/FilePacket.hpp")
    filedownlink = read("lib/fprime/Svc/FileDownlink/FileDownlink.cpp")
    interfaces = read("docs/interfaces.md")

    com_buffer = int(expect(r"FW_COM_BUFFER_MAX_SIZE = (\d+)", project_fp, "FW_COM_BUFFER_MAX_SIZE").group(1))
    file_buffer = int(expect(r"FW_FILE_BUFFER_MAX_SIZE = (\d+)", project_fp, "FW_FILE_BUFFER_MAX_SIZE").group(1))
    com_ccsds_file_buffer = int(
        expect(r"commsFileBuffSize\s*=\s*(\d+)", com_ccsds_cfg, "ComCcsds commsFileBuffSize").group(1)
    )
    obc_com_ccsds_file_buffer = int(
        expect(r"commsFileBuffSize\s*=\s*(\d+)", obc_com_ccsds_cfg, "OBCComCcsds commsFileBuffSize").group(1)
    )
    fw_id_size = int(expect(r"SIZE_OF_FwIdType = (\d+)", fp_config, "SIZE_OF_FwIdType").group(1))
    expect(r"SIZE_OF_FwOpcodeType = SIZE_OF_FwIdType", fp_config, "SIZE_OF_FwOpcodeType alias")
    opcode_size = fw_id_size
    packet_desc_size = int(expect(r"SIZE_OF_FwPacketDescriptorType = (\d+)", com_cfg, "SIZE_OF_FwPacketDescriptorType").group(1))
    header_len = int(expect(r"COMMAND_ENVELOPE_V1_HEADER_LENGTH\s*=\s*(\d+)", envelope, "envelope header length").group(1))
    mac_len = int(expect(r"COMMAND_ENVELOPE_V1_MAC_LENGTH\s*=\s*(\d+)", envelope, "envelope MAC length").group(1))
    header_expr = expect(
        r"Header\s*\{[\s\S]*?HEADERSIZE = (?P<expr>sizeof\(U8\)\s*\+\s*sizeof\(U32\))\s*\};",
        filepacket,
        "FilePacket::Header::HEADERSIZE expression",
    ).group("expr")
    header_size = evaluate_size_expr(header_expr, {})
    data_header_expr = expect(
        r"DataPacket\s*\{[\s\S]*?HEADERSIZE = (?P<expr>Header::HEADERSIZE\s*\+\s*sizeof\(U32\)\s*\+\s*sizeof\(U16\))\s*\};",
        filepacket,
        "FilePacket::DataPacket::HEADERSIZE expression",
    )
    data_header_size = evaluate_size_expr(data_header_expr.group("expr"), {"Header::HEADERSIZE": header_size})
    expect(r"maxDataSize\s*=\s*FILEDOWNLINK_INTERNAL_BUFFER_SIZE - Fw::FilePacket::DataPacket::HEADERSIZE - sizeof\(FwPacketDescriptorType\)", filedownlink, "FileDownlink maxDataSize formula")

    cmd_arg_budget = com_buffer - opcode_size - packet_desc_size
    envelope_overhead = header_len + mac_len
    inner_serialized = cmd_arg_budget - envelope_overhead
    inner_arg = inner_serialized - packet_desc_size - opcode_size
    file_data = file_buffer - data_header_size - packet_desc_size

    assert cmd_arg_budget == 506, cmd_arg_budget
    assert envelope_overhead == 60, envelope_overhead
    assert inner_serialized == 446, inner_serialized
    assert inner_arg == 440, inner_arg
    assert file_data == 2019, file_data
    assert com_ccsds_file_buffer == file_buffer, (com_ccsds_file_buffer, file_buffer)
    assert obc_com_ccsds_file_buffer == file_buffer, (obc_com_ccsds_file_buffer, file_buffer)

    apid_enum = parse_apid_enum(com_cfg)
    expected_apids = {
        "FW_PACKET_COMMAND": ("0x0000", "Command packet"),
        "FW_PACKET_TELEM": ("0x0001", "Telemetry packet"),
        "FW_PACKET_LOG": ("0x0002", "Log/event packet"),
        "FW_PACKET_FILE": ("0x0003", "File packet"),
        "FW_PACKET_PACKETIZED_TLM": ("0x0004", "Packetized telemetry"),
        "FW_PACKET_DP": ("0x0005", "Data product"),
        "FW_PACKET_IDLE": ("0x0006", "F' idle"),
        "FW_PACKET_HAND": ("0x00FE", "Handshake"),
        "FW_PACKET_UNKNOWN": ("0x00FF", "Unknown packet"),
        "SPP_IDLE_PACKET": ("0x07FF", "CCSDS idle packet"),
        "INVALID_UNINITIALIZED": ("0x0800", None),
    }
    assert set(apid_enum) == set(expected_apids), apid_enum
    for enum_name, (value, label) in expected_apids.items():
        assert apid_enum[enum_name] == value, f"{enum_name}={apid_enum[enum_name]} != {value}"
        if label is not None:
            expect_contains(f"| `{value}` | {label} |", interfaces, f"APID row {value}")
    expect_contains("| `>= 0x0800` | Invalid / uninitialized |", interfaces, "invalid APID boundary row")

    transport_rows = [
        "| `sband-primary` command ingress | `446` serialized inner `Fw::CmdPacket` bytes | `verified` | `FW_CMD_ARG_BUFFER_MAX_SIZE 506 - envelope overhead 60`; current repo-local source-derived ceiling | Path scope reused from hosted S-band primary command ingress evidence |",
        "| `sband-primary` command ingress | `440` inner command-argument bytes | `verified` | `446 - descriptor 2 - opcode 4`; readable derived inner-argument ceiling | Same current path as the governing serialized-inner ceiling above |",
        "| `uhf-backup` command ingress | `446` serialized inner `Fw::CmdPacket` bytes | `verified` | Same command-envelope formula as `sband-primary`; backup ingress uses the same current outer command budget | Path scope reused from bounded UHF backup command-ingress evidence |",
        "| `uhf-backup` command ingress | `440` inner command-argument bytes | `verified` | Same readable derived ceiling as `sband-primary` | Same current path as the governing serialized-inner ceiling above |",
        "| Active stock file/downlink packet ceiling | `2019` file-data bytes per `Fw::FilePacket::DATA` packet | `configured-hosted` | `FW_FILE_BUFFER_MAX_SIZE 2032 - DataPacket::HEADERSIZE 11 - descriptor 2`; stock current FileDownlink formula | Current global file-packet budget after the payload dual-artifact uplift; adjacent UHF throughput claims remain bounded separately |",
        "| Bounded reliable-transfer helper `DATA` segment | `160` bytes | `verified` | Current sidecar helper-path segment size only | Not a generic repo transport MTU; applies only to the bounded reliable-transfer sidecar path on default node `5` and explicit-switched node `6`; distinct from the stock UHF `243`-byte file/downlink ceiling |",
    ]
    for row in transport_rows:
        expect_contains(row, interfaces, f"transport row {row.split('|')[1].strip()}")

    summary_checks = [
        "| Broader target-flight MTU truth beyond the current governed paths | `residual-gap` | The repo now freezes only the current path-specific command and file ceilings; broader target-flight or cross-path MTU closure still needs later proof |",
        "| Future APID expansion beyond the current reservation map | `residual-gap` | The current reservation and proof split are frozen, but later active APID claims still require their own governed change and evidence |",
    ]
    for row in summary_checks:
        expect_contains(row, interfaces, f"summary row {row}")

    print("transport-mtu-apid-governance contract OK")
    print(f"command serialized ceiling: {inner_serialized}")
    print(f"command arg ceiling: {inner_arg}")
    print(f"file data ceiling: {file_data}")
    print(f"official file send-unit ceiling: {com_ccsds_file_buffer}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"contract check failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
