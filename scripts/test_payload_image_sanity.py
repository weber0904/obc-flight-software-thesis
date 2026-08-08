#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import sys
import unittest


ROOT_DIR = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT_DIR / "scripts" / "comm_verification" / "lib"))

from payload_image_sanity import _read_pnm_bytes
from payload_image_sanity import compute_luma_stats, evaluate_luma_sanity
from payload_image_sanity import evaluate_flip_effect_from_samples


class PayloadImageSanityTest(unittest.TestCase):
    def test_yuyv_black_frame_fails_threshold(self) -> None:
        width = 4
        height = 2
        raw = bytes(
            [
                0, 127, 0, 127,
                0, 127, 0, 127,
                0, 127, 0, 127,
                0, 127, 0, 127,
            ]
        )
        result = evaluate_luma_sanity(raw, "PIXEL_YUYV", width, height)
        self.assertFalse(result["passed"])
        self.assertLess(result["meanLuma"], 1.0)
        self.assertLess(result["p95Luma"], 8)

    def test_yuyv_bright_frame_passes_threshold(self) -> None:
        width = 4
        height = 2
        raw = bytes(
            [
                96, 127, 110, 127,
                128, 127, 140, 127,
                100, 127, 120, 127,
                130, 127, 150, 127,
            ]
        )
        result = evaluate_luma_sanity(raw, "PIXEL_YUYV", width, height)
        self.assertTrue(result["passed"])
        self.assertGreaterEqual(result["meanLuma"], 1.0)
        self.assertGreaterEqual(result["p95Luma"], 8)

    def test_rgb888_frame_passes_threshold(self) -> None:
        width = 2
        height = 2
        raw = bytes(
            [
                90, 80, 70,
                120, 110, 100,
                150, 140, 130,
                180, 170, 160,
            ]
        )
        result = evaluate_luma_sanity(raw, "PIXEL_RGB888", width, height)
        self.assertTrue(result["passed"])

    def test_nv12_stats_decode_y_plane(self) -> None:
        width = 4
        height = 2
        y_plane = bytes([10, 20, 30, 40, 50, 60, 70, 80])
        uv_plane = bytes([127, 127, 127, 127])
        stats = compute_luma_stats(y_plane + uv_plane, "PIXEL_NV12", width, height)
        self.assertEqual(8, stats["sampleCount"])
        self.assertEqual(80, stats["maxLuma"])
        self.assertEqual(10, stats["minLuma"])

    def test_flip_effect_detects_horizontal_match(self) -> None:
        width = 4
        height = 2
        reference = [
            10, 30, 90, 200,
            20, 40, 120, 220,
        ]
        candidate = [
            200, 90, 30, 10,
            220, 120, 40, 20,
        ]
        result = evaluate_flip_effect_from_samples(
            reference,
            candidate,
            width,
            height,
            expected_hflip=True,
            expected_vflip=False,
        )
        self.assertTrue(result["passed"])
        self.assertGreater(result["improvementRatio"], 1.15)

    def test_flip_effect_fails_without_expected_improvement(self) -> None:
        width = 4
        height = 2
        reference = [
            10, 20, 30, 40,
            50, 60, 70, 80,
        ]
        candidate = list(reference)
        result = evaluate_flip_effect_from_samples(
            reference,
            candidate,
            width,
            height,
            expected_hflip=True,
            expected_vflip=False,
        )
        self.assertFalse(result["passed"])

    def test_pgm_raster_may_start_with_whitespace_valued_pixel(self) -> None:
        pgm = b"P5\n2 2\n255\n" + bytes([10, 13, 32, 40])
        samples, width, height = _read_pnm_bytes(pgm)
        self.assertEqual((2, 2), (width, height))
        self.assertEqual([10, 13, 32, 40], samples)


if __name__ == "__main__":
    unittest.main()
