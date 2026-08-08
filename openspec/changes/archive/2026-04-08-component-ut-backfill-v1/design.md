## Overview

This change backfills the missing classic F' L2 layer for the remaining later true components. It does not replace the helper-level and integration-level tests already present in those slices.

## Design Decisions

### GpsBridge

- Keep `GpsSource` and `NmeaParser` as helper-layer logic with plain tests.
- Add a classic component tester for `GpsBridge` that drives commands and scheduler input through generated harness ports.

### StorageHealthBridge

- Keep `StorageScanner` as a helper-layer unit-tested module.
- Add a classic component tester for `StorageHealthBridge` that validates pre-configuration failure, commanded scans, scheduler cadence, telemetry publication, and warning/degraded events.

### HousekeepingArchive

- Keep `HousekeepingArchiveStore` as the helper-layer store with its own direct tests.
- Add a classic component tester for `HousekeepingArchive` that verifies command handling, `schedIn` capture cadence, and `fileOut` downlink request emission at the component surface.

### Test Shape

- All three backfills will use the same repo style as the existing early components:
  - `register_fprime_ut()`
  - `UT_AUTO_HELPERS ON`
  - generated `TesterBase` / `GTestBase`
  - small repo-owned `Tester.cpp`, `Tester.hpp`, and `TestMain.cpp`
