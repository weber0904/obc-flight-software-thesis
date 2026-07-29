## 1. GpsBridge Classic Harness

- [x] 1.1 Add `GpsBridge` classic F' UT CMake wiring with `register_fprime_ut()` and auto helpers.
- [x] 1.2 Add a classic tester covering `GPS_GET_STATE`, `GPS_SET_SOURCE_MODE`, valid/no-fix/rejected sentence behavior, source errors, and scheduler-driven polling.

## 2. StorageHealthBridge Classic Harness

- [x] 2.1 Add `StorageHealthBridge` classic F' UT CMake wiring with `register_fprime_ut()` and auto helpers.
- [x] 2.2 Add a classic tester covering `STORAGE_GET_STATUS`, `STORAGE_SCAN_NOW`, scheduler cadence, missing-root and scan-failed events, and telemetry publication.

## 3. HousekeepingArchive Classic Harness

- [x] 3.1 Add `HousekeepingArchive` classic F' UT CMake wiring with `register_fprime_ut()` and auto helpers.
- [x] 3.2 Add a classic tester covering `HK_CAPTURE_NOW`, `HK_DOWNLINK_INDEX`, `HK_DOWNLINK_SLOT`, command-response mapping, periodic capture cadence, and `fileOut` request emission.

## 4. Evidence And Validation

- [x] 4.1 Add a verification record summarizing the new classic harness coverage and the retained helper/integration layers.
- [x] 4.2 Run the new classic tester executables plus the existing helper/integration tests for GPS, storage health, and housekeeping archive.
- [x] 4.3 Run `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`.
- [x] 4.4 Run `openspec validate component-ut-backfill-v1` and `openspec validate --specs`.

## 5. Finalization

- [x] 5.1 Archive `component-ut-backfill-v1` after the new harnesses and evidence are aligned.
- [x] 5.2 Prepare the change for governed closeout on the dedicated feature branch.
