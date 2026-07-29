## 1. OpenSpec And Scope

- [x] 1.1 Create serial allocation governance artifacts for proposal, design, specs, and tasks.
- [x] 1.2 Keep this change docs/governance-only; do not implement GPS live UART.

## 2. Docs And Evidence

- [x] 2.1 Update comm and GPS specs to reserve `/dev/serial0` for comm and block GPS live UART until separate allocation.
- [x] 2.2 Update README, verification matrix, registry, and reporting package wording.
- [x] 2.3 Add `docs/test-records/serial-resource-allocation-governance-v1/README.md`.
- [x] 2.4 Pre-register the change in the reconciliation matrix for archive readiness.

## 3. Validation

- [x] 3.1 Run repo consistency checks.
- [x] 3.2 Run OpenSpec validation.
- [x] 3.3 Run the shared gate or record why this governance-only change reuses the release gate.
