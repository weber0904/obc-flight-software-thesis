## 1. Define shared FPP contracts

- [x] 1.1 Add the shared enums and constants for base ID ranges, CSP node IDs, and common modes under `OBC/Types/`
- [x] 1.2 Reconcile the narrative `core-system-contracts` source document with any newly explicit shared types

## 2. Implement the core components

- [x] 2.1 Add `ModeManager` with `MODE_*` commands and `SYS_*` mode/uptime telemetry
- [x] 2.2 Add `HealthMonitor` with `HEALTH_*` commands and `SYS_*` resource events/telemetry
- [x] 2.3 Add `CspBridge` with `CSP_*` commands plus CSP counter telemetry and diagnostic events

## 3. Add automated verification

- [x] 3.1 Add focused unit tests for `ModeManager`, `HealthMonitor`, and `CspBridge`
- [x] 3.2 Run the normal build and the unit-test build for the new core modules

## 4. Capture evidence and close the change

- [x] 4.1 Record the implementation and test results under `evidence/records/core-system-contracts-v1/`
- [x] 4.2 Run `openspec validate core-system-contracts-v1` and archive the change after the implementation and evidence are complete
