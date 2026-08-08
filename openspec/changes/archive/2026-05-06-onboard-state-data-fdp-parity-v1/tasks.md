## 1. OpenSpec And Contracts

- [x] 1.1 Create OpenSpec proposal, design, tasks, and delta specs for FDP parity and storage health.
- [x] 1.2 Validate the active change before implementation proceeds beyond code edits.

## 2. Storage Health Data Products Root

- [x] 2.1 Extend storage-health types, root enums, scanner configuration, and cached state with `data-products` root stats and observe-only policy fields.
- [x] 2.2 Publish `data-products` telemetry and include `DATA_PRODUCTS` in missing, scan-failed, and warning-threshold root events.
- [x] 2.3 Update storage scanner, bridge component tests, cached-state integration, and operator display coverage for the fifth root.

## 3. HK FDP V2 And HK Ring Fallback

- [x] 3.1 Migrate `HkTrendRecord` to V2 while preserving product record name/id and mapping data-products storage fields from the cached snapshot.
- [x] 3.2 Update HkTrendProductProducer L2 tests and helper tests for V2 payload version and new storage fields.
- [x] 3.3 Extend HousekeepingArchiveStore serialization to include the fifth root and policy/error fields, then bump archive file version to v3 and update archive tests.

## 4. Hosted FDP Parity Evidence

- [x] 4.1 Extend the hosted onboard-state-data probe to assert data-products storage health, GDS-received `.fdp` byte-match, and received-file decode.
- [x] 4.2 Update test record and verification-path registry with hosted official `.fdp` parity scope and explicit exclusions.
- [x] 4.3 Update verification matrix and repository consistency surfaces as needed.

## 5. Verification And Closeout

- [x] 5.1 Run focused tests for storage scanner/bridge, HK trend producer, onboard state data/source, DpCatalog gate, and HK archive storage integration.
- [x] 5.2 Run hosted onboard-state-data probe after a fresh build.
- [x] 5.3 Run full local gates: generate/build, UT generate/build, `fprime-util check --all`, OpenSpec validations, consistency checks, and verification CI script.
- [x] 5.4 Archive the OpenSpec change and prepare a review-ready Conventional Commit PR boundary.
