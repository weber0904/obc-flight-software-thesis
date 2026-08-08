## 1. Artifact Completion

- [x] 1.1 Finalize `gps-subsystem`, `storage-health`, and `housekeeping-archive` delta specs for Copilot follow-up fixes

## 2. GPS Parser Hardening

- [x] 2.1 Add explicit first-version NMEA sentence-length and field-count bounds to the parser implementation
- [x] 2.2 Extend GPS parser tests to cover oversized or over-field-count input rejection

## 3. Storage Health Diagnostics

- [x] 3.1 Distinguish missing roots from unreadable / scan-failure roots in the storage scanner
- [x] 3.2 Propagate meaningful per-root scan failure codes into the storage-health runtime path and emitted events
- [x] 3.3 Extend storage-health tests and hosted validation to cover the refined failure semantics

## 4. Archive Versioning And Evidence Cleanup

- [x] 4.1 Bump housekeeping archive file version to match the storage-health record-layout extension and cover it in tests
- [x] 4.2 Fix archived evidence links and verification-path-registry documentation follow-ups called out by review

## 5. Verification And Closeout

- [x] 5.1 Run the local verification gate plus GPS and storage-health focused tests/probes
- [x] 5.2 Record the follow-up evidence scope, validate the change, and prepare it for archive
