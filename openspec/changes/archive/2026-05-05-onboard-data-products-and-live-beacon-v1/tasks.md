## 1. Data Model And Helpers

- [x] 1.1 Define `ReducedStateV1`, `BeaconV1`, CRC, and decode helpers for onboard state and live beacon data.
- [x] 1.2 Define FPP serializable HK trend record types with raw cached subsystem fields and validity flags.
- [x] 1.3 Remove repo-local OPD1 product/catalog helper ownership from the mission baseline.
- [x] 1.4 Add or update L1 tests for reduced-state classification, beacon round-trip decode, CRC rejection, and HK trend field mapping.

## 2. Flight Components

- [x] 2.1 Add/retain `OnboardStateMonitor` with `IStateSnapshotSource` dependency and cached reduced-state invalidation on source failure.
- [x] 2.2 Refactor `BeaconPublisher` to use `IReducedStateSource` and `IBeaconSink`, keep 17-tick cadence, distinguish not configured from source unavailable, and advance sequence only after successful sink send.
- [x] 2.3 Add `HkTrendProductProducer` with official F' product get/send ports, 30-tick cadence, source-unavailable handling, and DP buffer/serialize failure handling.
- [x] 2.4 Remove `StateDataArchive` from build/runtime baseline.
- [x] 2.5 Add or refresh classic F' L2 harness coverage for each new or modified real component.

## 3. Topology And Runtime Integration

- [x] 3.1 Wire official `DpManager`, `DpWriter`, `DpCatalog`, and DP buffer manager into the OBC topology.
- [x] 3.2 Connect HK trend product ports to `DpManager`, `DpWriter`, and `DpCatalog` downlink behavior through existing `FileHandling.fileDownlink`.
- [x] 3.3 Configure runtime-root `<runtime-root>/data-products/` paths for official data product files and catalog state.
- [x] 3.4 Configure a COMM-facing live beacon sink while preserving existing `ComFprime`, command, event, telemetry, and `HousekeepingArchive` paths.

## 4. Hosted Probes And Evidence

- [x] 4.1 Run hosted probe that validates cached subsystem state to reduced state, live beacon decode, official HK trend data-product file generation, and catalog behavior.
- [x] 4.2 Update debug tooling/evidence so beacon capture is verification-only and not a mission `BEACON_HISTORY` data product.
- [x] 4.3 Update `docs/test-records/onboard-data-products-and-live-beacon-v1/README.md`.
- [x] 4.4 Update `docs/verification-matrix.md` and `docs/verification-path-registry.md`.

## 5. Verification And Governance

- [x] 5.1 Run `fprime-util generate -f`, `fprime-util build`, `fprime-util generate --ut -f`, `fprime-util build --ut`, and focused helper/component tests.
- [x] 5.2 Run `openspec validate onboard-data-products-and-live-beacon-v1`.
- [x] 5.3 Run `openspec validate --specs`.
- [x] 5.4 Run `bash scripts/run_verification_ci.sh`.
- [x] 5.5 Prepare clean replacement PR, close PR #41 as superseded, and reference the new PR from the old one.
