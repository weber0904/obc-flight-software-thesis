#!/usr/bin/env python3
from __future__ import annotations

import math
import pathlib
import subprocess
from typing import Iterable


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def _p95(values: list[int]) -> int:
    _require(bool(values), "cannot compute percentile from empty luma sample set")
    ordered = sorted(values)
    index = max(0, math.ceil(len(ordered) * 0.95) - 1)
    return int(ordered[index])


def _rgb_luma(r: int, g: int, b: int) -> int:
    return int(round((0.299 * r) + (0.587 * g) + (0.114 * b)))


def _decode_luma_samples(raw_bytes: bytes, pixel_format: str, width: int, height: int) -> list[int]:
    if pixel_format == "PIXEL_YUYV":
        return list(_iter_yuyv_luma(raw_bytes, width, height))
    if pixel_format == "PIXEL_UYVY":
        return list(_iter_uyvy_luma(raw_bytes, width, height))
    if pixel_format == "PIXEL_NV12":
        return list(_iter_nv12_luma(raw_bytes, width, height))
    if pixel_format == "PIXEL_RGB888":
        return list(_iter_rgb888_luma(raw_bytes, width, height, bgr=False))
    if pixel_format == "PIXEL_BGR888":
        return list(_iter_rgb888_luma(raw_bytes, width, height, bgr=True))
    raise ValueError(f"unsupported payload pixel format: {pixel_format}")


def _normalized_samples(samples: list[int]) -> list[float]:
    _require(bool(samples), "cannot normalize empty sample set")
    mean = sum(samples) / len(samples)
    variance = sum((float(value) - mean) ** 2 for value in samples) / len(samples)
    stddev = math.sqrt(variance)
    if stddev < 1e-6:
        return [0.0 for _ in samples]
    return [(float(value) - mean) / stddev for value in samples]


def _flip_samples(samples: list[float], width: int, height: int, *, hflip: bool, vflip: bool) -> list[float]:
    _require(width > 0 and height > 0, "flip width/height must be positive")
    _require(len(samples) == width * height, "sample count does not match width*height")
    flipped = [0.0 for _ in samples]
    for row in range(height):
        source_row = height - 1 - row if vflip else row
        for column in range(width):
            source_column = width - 1 - column if hflip else column
            flipped[row * width + column] = samples[source_row * width + source_column]
    return flipped


def _mean_absolute_difference(reference: list[float], candidate: list[float]) -> float:
    _require(len(reference) == len(candidate), "reference/candidate length mismatch")
    return sum(abs(ref - cand) for ref, cand in zip(reference, candidate)) / len(reference)


def evaluate_flip_effect_from_samples(
    reference_samples: list[int],
    candidate_samples: list[int],
    width: int,
    height: int,
    *,
    expected_hflip: bool,
    expected_vflip: bool,
    min_improvement_ratio: float = 1.15,
) -> dict[str, float | int | bool]:
    _require(len(reference_samples) == len(candidate_samples), "reference/candidate sample count mismatch")
    normalized_reference = _normalized_samples(reference_samples)
    normalized_candidate = _normalized_samples(candidate_samples)
    baseline_difference = _mean_absolute_difference(normalized_reference, normalized_candidate)
    corrected_difference = _mean_absolute_difference(
        normalized_reference,
        _flip_samples(
            normalized_candidate,
            width,
            height,
            hflip=expected_hflip,
            vflip=expected_vflip,
        ),
    )
    improvement_ratio = math.inf if corrected_difference <= 1e-9 else baseline_difference / corrected_difference
    passed = baseline_difference > corrected_difference and improvement_ratio >= min_improvement_ratio
    return {
        "width": width,
        "height": height,
        "baselineDifference": baseline_difference,
        "correctedDifference": corrected_difference,
        "improvementRatio": improvement_ratio,
        "expectedHflip": expected_hflip,
        "expectedVflip": expected_vflip,
        "minImprovementRatio": min_improvement_ratio,
        "passed": passed,
    }


def _read_pnm_bytes(pnm_bytes: bytes) -> tuple[list[int], int, int]:
    _require(len(pnm_bytes) >= 2, "PNM payload is too short")
    magic = pnm_bytes[:2]
    _require(magic in {b"P5", b"P6"}, f"unsupported PNM magic {magic!r}")
    cursor = 2
    tokens: list[bytes] = []
    while len(tokens) < 3:
        while cursor < len(pnm_bytes) and chr(pnm_bytes[cursor]).isspace():
            cursor += 1
        _require(cursor < len(pnm_bytes), "unexpected EOF while parsing PNM header")
        if pnm_bytes[cursor:cursor + 1] == b"#":
            while cursor < len(pnm_bytes) and pnm_bytes[cursor:cursor + 1] not in {b"\n", b"\r"}:
                cursor += 1
            continue
        token_start = cursor
        while cursor < len(pnm_bytes) and not chr(pnm_bytes[cursor]).isspace():
            cursor += 1
        tokens.append(pnm_bytes[token_start:cursor])
    width = int(tokens[0])
    height = int(tokens[1])
    max_value = int(tokens[2])
    _require(max_value == 255, "only 8-bit PNM images are supported")
    _require(cursor < len(pnm_bytes), "missing PNM raster separator")
    separator = pnm_bytes[cursor]
    _require(chr(separator).isspace(), "missing whitespace between PNM header and raster")
    cursor += 1
    if separator == ord("\r") and cursor < len(pnm_bytes) and pnm_bytes[cursor] == ord("\n"):
        cursor += 1
    raster = pnm_bytes[cursor:]
    if magic == b"P5":
        _require(len(raster) >= width * height, "PGM raster is truncated")
        return list(raster[: width * height]), width, height
    _require(len(raster) >= width * height * 3, "PPM raster is truncated")
    luma: list[int] = []
    for offset in range(0, width * height * 3, 3):
        luma.append(_rgb_luma(raster[offset], raster[offset + 1], raster[offset + 2]))
    return luma, width, height


def decode_jpeg_luma_samples(jpeg_path: pathlib.Path) -> tuple[list[int], int, int]:
    result = subprocess.run(
        ["djpeg", "-grayscale", "-pnm", str(jpeg_path)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        timeout=30.0,
    )
    if result.returncode != 0:
        raise ValueError(
            f"djpeg failed for {jpeg_path}: {result.stderr.decode(errors='replace').strip()}"
        )
    return _read_pnm_bytes(result.stdout)


def evaluate_preview_flip_effect(
    reference_preview_path: pathlib.Path,
    candidate_preview_path: pathlib.Path,
    *,
    expected_hflip: bool,
    expected_vflip: bool,
    min_improvement_ratio: float = 1.15,
) -> dict[str, float | int | bool]:
    reference_samples, width, height = decode_jpeg_luma_samples(reference_preview_path)
    candidate_samples, candidate_width, candidate_height = decode_jpeg_luma_samples(candidate_preview_path)
    _require(width == candidate_width and height == candidate_height, "preview dimension mismatch")
    return evaluate_flip_effect_from_samples(
        reference_samples,
        candidate_samples,
        width,
        height,
        expected_hflip=expected_hflip,
        expected_vflip=expected_vflip,
        min_improvement_ratio=min_improvement_ratio,
    )


def _iter_yuyv_luma(raw_bytes: bytes, width: int, height: int) -> Iterable[int]:
    _require(width % 2 == 0, "YUYV width must be even")
    _require(height > 0, "height must be positive")
    _require(len(raw_bytes) % height == 0, "YUYV raw size must divide evenly by height")
    stride = len(raw_bytes) // height
    _require(stride >= width * 2, "YUYV stride is smaller than width*2")
    for row in range(height):
        row_offset = row * stride
        for column in range(0, width, 2):
            pixel_offset = row_offset + (column * 2)
            yield raw_bytes[pixel_offset + 0]
            yield raw_bytes[pixel_offset + 2]


def _iter_uyvy_luma(raw_bytes: bytes, width: int, height: int) -> Iterable[int]:
    _require(width % 2 == 0, "UYVY width must be even")
    _require(height > 0, "height must be positive")
    _require(len(raw_bytes) % height == 0, "UYVY raw size must divide evenly by height")
    stride = len(raw_bytes) // height
    _require(stride >= width * 2, "UYVY stride is smaller than width*2")
    for row in range(height):
        row_offset = row * stride
        for column in range(0, width, 2):
            pixel_offset = row_offset + (column * 2)
            yield raw_bytes[pixel_offset + 1]
            yield raw_bytes[pixel_offset + 3]


def _iter_nv12_luma(raw_bytes: bytes, width: int, height: int) -> Iterable[int]:
    _require(width > 0 and height > 0, "NV12 width/height must be positive")
    _require((len(raw_bytes) * 2) % (height * 3) == 0, "NV12 raw size is not compatible with height")
    stride = (len(raw_bytes) * 2) // (height * 3)
    _require(stride >= width, "NV12 inferred stride is smaller than width")
    y_plane_size = stride * height
    _require(y_plane_size <= len(raw_bytes), "NV12 inferred Y plane exceeds buffer length")
    y_plane = raw_bytes[:y_plane_size]
    for row in range(height):
        row_offset = row * stride
        for column in range(width):
            yield y_plane[row_offset + column]


def _iter_rgb888_luma(raw_bytes: bytes, width: int, height: int, *, bgr: bool) -> Iterable[int]:
    _require(width > 0 and height > 0, "RGB width/height must be positive")
    _require(len(raw_bytes) % height == 0, "RGB raw size must divide evenly by height")
    stride = len(raw_bytes) // height
    _require(stride >= width * 3, "RGB stride is smaller than width*3")
    for row in range(height):
        row_offset = row * stride
        for column in range(width):
            pixel_offset = row_offset + (column * 3)
            c0 = raw_bytes[pixel_offset + 0]
            c1 = raw_bytes[pixel_offset + 1]
            c2 = raw_bytes[pixel_offset + 2]
            if bgr:
                yield _rgb_luma(c2, c1, c0)
            else:
                yield _rgb_luma(c0, c1, c2)


def compute_luma_stats(raw_bytes: bytes, pixel_format: str, width: int, height: int) -> dict[str, float | int | str]:
    luma = _decode_luma_samples(raw_bytes, pixel_format, width, height)
    _require(bool(luma), "no luma samples decoded from raw artifact")
    return {
        "pixelFormat": pixel_format,
        "width": width,
        "height": height,
        "sampleCount": len(luma),
        "meanLuma": sum(luma) / len(luma),
        "minLuma": min(luma),
        "maxLuma": max(luma),
        "p95Luma": _p95(luma),
    }


def evaluate_luma_sanity(
    raw_bytes: bytes,
    pixel_format: str,
    width: int,
    height: int,
    *,
    mean_threshold: float = 1.0,
    p95_threshold: int = 8,
) -> dict[str, float | int | str | bool]:
    stats = compute_luma_stats(raw_bytes, pixel_format, width, height)
    passed = float(stats["meanLuma"]) >= mean_threshold and int(stats["p95Luma"]) >= p95_threshold
    return {
        **stats,
        "meanThreshold": mean_threshold,
        "p95Threshold": p95_threshold,
        "passed": passed,
    }
