#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess
from dataclasses import dataclass
from typing import Any


LEGACY_HEADER_SUFFIX = ".PayloadCaptureHeader"
LEGACY_BYTES_SUFFIX = ".PayloadCaptureJpegBytes"
ARTIFACT_HEADER_SUFFIX = ".PayloadCaptureArtifactHeader"
ARTIFACT_BYTES_SUFFIX = ".PayloadCaptureArtifactBytes"


@dataclass(frozen=True)
class PayloadFormat:
    family: str
    header_suffix: str
    bytes_suffix: str


LEGACY_FORMAT = PayloadFormat("legacy-jpeg-v1", LEGACY_HEADER_SUFFIX, LEGACY_BYTES_SUFFIX)
ARTIFACT_FORMAT = PayloadFormat("artifact-v2", ARTIFACT_HEADER_SUFFIX, ARTIFACT_BYTES_SUFFIX)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Decode payload capture .fdp families, extract the payload artifact, and verify bounded metadata."
    )
    parser.add_argument("--fdp-file", required=True, help="Path to the payload .fdp file")
    parser.add_argument("--dictionary", required=True, help="Path to the F' JSON dictionary")
    parser.add_argument("--output-dir", required=True, help="Directory for decode artifacts")
    parser.add_argument(
        "--dp-writer",
        default="fprime-dp-write",
        help="Path to fprime-dp-write (default: fprime-dp-write from PATH)",
    )
    parser.add_argument("--source-artifact", help="Optional source artifact for SHA-256 comparison")
    parser.add_argument("--expected-source-artifact-sha256")
    parser.add_argument("--source-jpeg", help="Deprecated alias for --source-artifact")
    parser.add_argument("--expected-source-jpeg-sha256", help="Deprecated alias for --expected-source-artifact-sha256")
    parser.add_argument("--expected-capture-id", type=int)
    parser.add_argument("--expected-capture-index", type=int)
    parser.add_argument("--expected-artifact-kind")
    parser.add_argument("--expected-relative-path")
    parser.add_argument("--expected-relative-data-product-path")
    parser.add_argument("--expected-resolution")
    parser.add_argument("--expected-capture-policy")
    parser.add_argument(
        "--expected-session-kind",
        dest="expected_capture_policy_legacy",
        help="Deprecated alias for --expected-capture-policy",
    )
    parser.add_argument("--require-published", action="store_true")
    parser.add_argument("--require-valid-jpeg", action="store_true")
    return parser.parse_args()


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def flatten_struct_members(value: Any) -> Any:
    if isinstance(value, list):
        if all(isinstance(item, dict) and len(item) == 1 for item in value):
            flattened: dict[str, Any] = {}
            for item in value:
                key, child = next(iter(item.items()))
                flattened[key] = flatten_struct_members(child)
            return flattened
        return [flatten_struct_members(item) for item in value]
    if isinstance(value, dict):
        return {key: flatten_struct_members(child) for key, child in value.items()}
    return value


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def decode_fdp(
    dp_writer: str,
    fdp_file: pathlib.Path,
    dictionary: pathlib.Path,
    output_dir: pathlib.Path,
) -> pathlib.Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    log_path = output_dir / "decode.log"
    args = [dp_writer, str(fdp_file), str(dictionary)]
    with log_path.open("w", encoding="utf-8") as log:
        log.write("$ " + " ".join(args) + "\n")
        log.flush()
        result = subprocess.run(args, cwd=output_dir, stdout=log, stderr=subprocess.STDOUT, text=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"fprime-dp-write failed for {fdp_file}; see {log_path}")
    decoded_json = output_dir / f"{fdp_file.stem}.json"
    require(decoded_json.is_file(), f"decoded JSON was not produced: {decoded_json}")
    return decoded_json


def load_record_name_by_id(dictionary: dict[str, Any]) -> dict[int, str]:
    mapping: dict[int, str] = {}
    for record in dictionary.get("records", []):
        record_id = record.get("id")
        name = record.get("name")
        if isinstance(record_id, int) and isinstance(name, str):
            mapping[record_id] = name
    return mapping


def detect_payload_format(decoded: list[dict[str, Any]], record_name_by_id: dict[int, str]) -> PayloadFormat:
    record_names = {
        record_name_by_id.get(entry.get("dataId"), "")
        for entry in decoded[1:]
        if isinstance(entry.get("dataId"), int)
    }
    if any(name.endswith(ARTIFACT_HEADER_SUFFIX) for name in record_names) and any(
        name.endswith(ARTIFACT_BYTES_SUFFIX) for name in record_names
    ):
        return ARTIFACT_FORMAT
    if any(name.endswith(LEGACY_HEADER_SUFFIX) for name in record_names) and any(
        name.endswith(LEGACY_BYTES_SUFFIX) for name in record_names
    ):
        return LEGACY_FORMAT
    raise RuntimeError("decoded .fdp did not contain a recognized payload record family")


def find_record(decoded: list[dict[str, Any]], record_name_by_id: dict[int, str], suffix: str) -> tuple[dict[str, Any], str]:
    for entry in decoded[1:]:
        record_id = entry.get("dataId")
        if not isinstance(record_id, int):
            continue
        name = record_name_by_id.get(record_id, "")
        if name.endswith(suffix):
            return entry, name
    raise RuntimeError(f"decoded .fdp did not contain a record ending with {suffix!r}")


def find_records(decoded: list[dict[str, Any]], record_name_by_id: dict[int, str], suffix: str) -> tuple[list[dict[str, Any]], str]:
    matches: list[dict[str, Any]] = []
    record_name = ""
    for entry in decoded[1:]:
        record_id = entry.get("dataId")
        if not isinstance(record_id, int):
            continue
        name = record_name_by_id.get(record_id, "")
        if name.endswith(suffix):
            matches.append(entry)
            record_name = name
    if not matches:
        raise RuntimeError(f"decoded .fdp did not contain a record ending with {suffix!r}")
    return matches, record_name


def is_valid_jpeg(data: bytes) -> bool:
    return len(data) >= 4 and data[:2] == b"\xff\xd8" and data[-2:] == b"\xff\xd9"


def value_as_int(value: Any, default: int = -1) -> int:
    return int(value) if isinstance(value, int) else default


def artifact_descriptor_from_header(header: dict[str, Any], payload_format: PayloadFormat) -> dict[str, Any]:
    if payload_format == LEGACY_FORMAT:
        return {
            "artifactKind": "PREVIEW_JPEG",
            "relativePath": header.get("relativePath"),
            "relativeDataProductPath": header.get("relativeDataProductPath"),
            "published": bool(header.get("dataProductPublished")),
            "artifactBytesField": "jpegBytes",
            "artifactExtension": ".jpg",
        }

    artifact_kind = header.get("artifactKind")
    require(isinstance(artifact_kind, str), f"artifact header did not decode artifactKind into a string: {artifact_kind!r}")
    if artifact_kind == "RAW_FRAME":
        return {
            "artifactKind": artifact_kind,
            "relativePath": header.get("rawRelativePath"),
            "relativeDataProductPath": header.get("rawDataProductRelativePath"),
            "published": bool(header.get("rawDataProductPublished")),
            "artifactBytesField": "rawBytes",
            "artifactExtension": ".bin",
        }
    if artifact_kind == "PREVIEW_JPEG":
        return {
            "artifactKind": artifact_kind,
            "relativePath": header.get("previewRelativePath"),
            "relativeDataProductPath": header.get("previewDataProductRelativePath"),
            "published": bool(header.get("previewDataProductPublished")),
            "artifactBytesField": "previewJpegBytes",
            "artifactExtension": ".jpg",
        }
    raise RuntimeError(f"unsupported payload artifactKind {artifact_kind!r}")


def decode_payload_slice(
    *,
    dp_writer: str,
    fdp_file: pathlib.Path,
    dictionary_path: pathlib.Path,
    output_dir: pathlib.Path,
) -> dict[str, Any]:
    decoded_path = decode_fdp(dp_writer, fdp_file, dictionary_path, output_dir)
    dictionary = json.loads(dictionary_path.read_text(encoding="utf-8"))
    decoded = json.loads(decoded_path.read_text(encoding="utf-8"))
    require(isinstance(decoded, list) and len(decoded) >= 3, "decoded payload .fdp was unexpectedly short")

    record_name_by_id = load_record_name_by_id(dictionary)
    payload_format = detect_payload_format(decoded, record_name_by_id)
    header_record, header_record_name = find_record(decoded, record_name_by_id, payload_format.header_suffix)
    bytes_records, bytes_record_name = find_records(decoded, record_name_by_id, payload_format.bytes_suffix)

    header = flatten_struct_members(header_record.get("data"))
    require(isinstance(header, dict), "payload header record did not decode into a struct dictionary")
    descriptor = artifact_descriptor_from_header(header, payload_format)

    artifact_chunks: list[bytes] = []
    for index, bytes_record in enumerate(bytes_records):
        chunk = bytes_record.get("data")
        require(isinstance(chunk, list), f"payload bytes record {index} did not decode into a byte array")
        require(all(isinstance(item, int) for item in chunk), f"payload bytes record {index} contained non-byte elements")
        artifact_chunks.append(bytes(chunk))

    return {
        "fdpFile": str(fdp_file),
        "fdpSha256": sha256_file(fdp_file),
        "decodedJson": str(decoded_path),
        "formatFamily": payload_format.family,
        "headerRecordName": header_record_name,
        "bytesRecordName": bytes_record_name,
        "header": header,
        "descriptor": descriptor,
        "artifactBytes": b"".join(artifact_chunks),
        "artifactRecordCount": len(bytes_records),
    }


def family_identity_key(slice_summary: dict[str, Any]) -> tuple[Any, ...]:
    header = slice_summary["header"]
    descriptor = slice_summary["descriptor"]
    return (
        header.get("captureId"),
        header.get("captureIndex"),
        descriptor.get("artifactKind"),
        descriptor.get("relativePath"),
    )


def main() -> int:
    args = parse_args()
    expected_capture_policy = args.expected_capture_policy
    if expected_capture_policy is None:
        expected_capture_policy = args.expected_capture_policy_legacy

    fdp_file = pathlib.Path(args.fdp_file).resolve()
    dictionary_path = pathlib.Path(args.dictionary).resolve()
    output_dir = pathlib.Path(args.output_dir).resolve()
    source_artifact = pathlib.Path(args.source_artifact).resolve() if args.source_artifact else None
    if source_artifact is None and args.source_jpeg:
        source_artifact = pathlib.Path(args.source_jpeg).resolve()
    expected_source_artifact_sha = args.expected_source_artifact_sha256 or args.expected_source_jpeg_sha256

    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    primary_slice = decode_payload_slice(
        dp_writer=args.dp_writer,
        fdp_file=fdp_file,
        dictionary_path=dictionary_path,
        output_dir=output_dir / "decode-primary",
    )
    header = primary_slice["header"]
    descriptor = primary_slice["descriptor"]

    family_slices: list[dict[str, Any]] = []
    family_root = output_dir / "family-members"
    expected_identity = family_identity_key(primary_slice)
    for sibling in sorted(fdp_file.parent.glob("*.fdp")):
        try:
            slice_summary = decode_payload_slice(
                dp_writer=args.dp_writer,
                fdp_file=sibling,
                dictionary_path=dictionary_path,
                output_dir=family_root / sibling.stem,
            )
        except RuntimeError:
            continue
        if family_identity_key(slice_summary) != expected_identity:
            continue
        family_slices.append(slice_summary)

    require(family_slices, f"did not discover any payload .fdp family members for {fdp_file}")
    family_slices.sort(key=lambda item: str(item["descriptor"]["relativeDataProductPath"]))
    artifact_bytes = b"".join(bytes(item["artifactBytes"]) for item in family_slices)
    family_published = any(bool(item["descriptor"].get("published")) for item in family_slices)
    expected_size = value_as_int(header.get(descriptor["artifactBytesField"]))
    require(
        expected_size == len(artifact_bytes),
        f"payload artifact length mismatch: header={expected_size} extracted={len(artifact_bytes)}",
    )

    extracted_artifact = output_dir / f"{fdp_file.stem}{descriptor['artifactExtension']}"
    extracted_artifact.write_bytes(artifact_bytes)

    artifact_kind = str(descriptor["artifactKind"])
    valid_jpeg = artifact_kind == "PREVIEW_JPEG" and is_valid_jpeg(artifact_bytes)
    if args.require_valid_jpeg or artifact_kind == "PREVIEW_JPEG":
        require(valid_jpeg, f"preview payload artifact is not a valid JPEG: {extracted_artifact}")

    if args.expected_capture_id is not None:
        require(
            value_as_int(header.get("captureId")) == args.expected_capture_id,
            f"captureId mismatch: expected {args.expected_capture_id}, got {header.get('captureId')}",
        )
    if args.expected_capture_index is not None:
        require(
            value_as_int(header.get("captureIndex")) == args.expected_capture_index,
            f"captureIndex mismatch: expected {args.expected_capture_index}, got {header.get('captureIndex')}",
        )
    if args.expected_artifact_kind is not None:
        require(
            artifact_kind == args.expected_artifact_kind,
            f"artifactKind mismatch: expected {args.expected_artifact_kind}, got {artifact_kind}",
        )
    if args.expected_relative_path is not None:
        require(
            descriptor.get("relativePath") == args.expected_relative_path,
            f"relativePath mismatch: expected {args.expected_relative_path}, got {descriptor.get('relativePath')}",
        )
    if args.expected_relative_data_product_path is not None:
        require(
            descriptor.get("relativeDataProductPath") == args.expected_relative_data_product_path,
            "relativeDataProductPath mismatch: "
            f"expected {args.expected_relative_data_product_path}, got {descriptor.get('relativeDataProductPath')}",
        )
    if args.expected_resolution is not None:
        require(
            header.get("resolution") == args.expected_resolution,
            f"resolution mismatch: expected {args.expected_resolution}, got {header.get('resolution')}",
        )
    header_capture_policy = header.get("capturePolicy", header.get("sessionKind"))
    if expected_capture_policy is not None:
        require(
            header_capture_policy == expected_capture_policy,
            f"capturePolicy mismatch: expected {expected_capture_policy}, got {header_capture_policy}",
        )
    if args.require_published:
        require(family_published is True, "payload header family did not report published=true for this artifact")

    extracted_artifact_sha = sha256_file(extracted_artifact)
    source_artifact_sha = None
    if source_artifact is not None:
        require(source_artifact.is_file(), f"source artifact does not exist: {source_artifact}")
        source_artifact_sha = sha256_file(source_artifact)
        require(
            extracted_artifact_sha == source_artifact_sha,
            f"extracted artifact SHA-256 mismatch: source={source_artifact_sha} extracted={extracted_artifact_sha}",
        )
    elif expected_source_artifact_sha is not None:
        source_artifact_sha = expected_source_artifact_sha
        require(
            extracted_artifact_sha == source_artifact_sha,
            f"extracted artifact SHA-256 mismatch: expected={source_artifact_sha} extracted={extracted_artifact_sha}",
        )

    summary = {
        "fdpFile": str(fdp_file),
        "fdpSha256": primary_slice["fdpSha256"],
        "decodedJson": primary_slice["decodedJson"],
        "formatFamily": primary_slice["formatFamily"],
        "headerRecordName": primary_slice["headerRecordName"],
        "bytesRecordName": primary_slice["bytesRecordName"],
        "artifactRecordCount": primary_slice["artifactRecordCount"],
        "captureId": header.get("captureId"),
        "captureIndex": header.get("captureIndex"),
        "artifactKind": artifact_kind,
        "relativePath": descriptor.get("relativePath"),
        "relativeDataProductPath": descriptor.get("relativeDataProductPath"),
        "published": family_published,
        "resolution": header.get("resolution"),
        "capturePolicy": header_capture_policy,
        "sessionKind": header.get("sessionKind"),
        "pixelFormat": header.get("pixelFormat", "PIXEL_UNKNOWN"),
        "imageWidth": header.get("imageWidth"),
        "imageHeight": header.get("imageHeight"),
        "rawBytes": header.get("rawBytes", 0),
        "previewJpegBytes": header.get("previewJpegBytes", header.get("jpegBytes", 0)),
        "artifactBytes": len(artifact_bytes),
        "artifactExtension": descriptor["artifactExtension"],
        "validJpeg": valid_jpeg,
        "extractedArtifact": str(extracted_artifact),
        "extractedArtifactSha256": extracted_artifact_sha,
        "sourceArtifactSha256": source_artifact_sha,
        "familyFdpFiles": [slice_summary["fdpFile"] for slice_summary in family_slices],
        "familyFdpSha256": [slice_summary["fdpSha256"] for slice_summary in family_slices],
        "header": header,
    }
    summary_path = output_dir / "payload-fdp-summary.json"
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"format-family={summary['formatFamily']}")
    print(f"artifact-kind={summary['artifactKind']}")
    print(f"family-fdp-count={len(summary['familyFdpFiles'])}")
    print(f"artifact-bytes={summary['artifactBytes']}")
    print(f"extracted-artifact={summary['extractedArtifact']}")
    print(f"extracted-artifact-sha256={summary['extractedArtifactSha256']}")
    if source_artifact_sha is not None:
        print(f"source-artifact-sha256={source_artifact_sha}")
    print(f"payload-fdp-summary={summary_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
