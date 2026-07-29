## 1. OpenSpec And Scope

- [x] 1.1 Add proposal, design, spec deltas, and tasks for the carrier/backend abstraction slice.
- [x] 1.2 Keep F' business-layer contracts and subsystem-owned CSP protocols unchanged.

## 2. Runtime Abstraction

- [x] 2.1 Refactor the libcsp runtime config to add an explicit transport/backend selector.
- [x] 2.2 Move ZMQHUB-specific binding logic behind a backend abstraction while preserving existing defaults.
- [x] 2.3 Update `CspBridge`, EPS/ADCS transport setup, and related scripts to consume the new config shape.

## 3. Documentation And Validation

- [x] 3.1 Update README and relevant formal specs to describe `ZMQHUB over TCP/IP` as the current development carrier.
- [x] 3.2 Add change-level evidence describing the abstraction boundary and rerun the existing CSP/EPS/ADCS checks.
- [x] 3.3 Run `openspec validate csp-link-backend-abstraction-v1` and `openspec validate --specs`.
