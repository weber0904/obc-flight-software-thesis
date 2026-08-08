## Why

The hosted internal CSP foundation now exists, but EPS business traffic still uses the legacy project-local direct ZMQ request/response path. This change proves the first real subsystem vertical slice over libcsp so the repository starts moving from foundation diagnostics to actual internal subsystem communication.

## What Changes

- Replace the default EPS bridge transport with a libcsp-backed EPS service client while preserving the public `EPS_*` F' command, telemetry, and event contract.
- Rework the hosted EPS simulator into libcsp node `2` with EPS-owned CSP services on application ports `10` through `13`, leaving libcsp reserved ports `0` through `3` available for built-in management services.
- Retain `EpsSimModel` state/business logic and the existing `IEpsTransport` abstraction.
- Retire the active `eps_zmq_integration_test` path in favor of a governed `eps_csp_integration_test`.
- Keep ADCS on the temporary legacy direct-ZMQ path until `adcs-csp-vertical-slice-v1`.
- Do not change the GDS ground path, external comm path, GPS path, mission logic, boot/update logic, or housekeeping archive semantics except for consuming the same EPS public state after transport migration.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `eps-subsystem`: EPS hosted simulator and `EpsBridge` default transport move from project-local direct ZMQ request/response to EPS-owned libcsp service payloads on node `2`, ports `10` through `13`.
- `verification-evidence`: EPS CSP vertical-slice evidence must distinguish the new internal libcsp EPS path from the foundation-only CSP path, the ground path, and the still-legacy ADCS path.
- `verification-path-registry`: register the hosted EPS internal libcsp path as a proven path once the vertical slice passes.

## Impact

- Affected code: EPS transport implementation, EPS simulator server/main, simulator CMake registration, hosted stack startup, EPS integration tests, and verification evidence.
- Affected systems: hosted internal EPS subsystem path only.
- Explicitly out of scope: ADCS migration, comm/external radio migration, GPS live path, ground CSP gateway/tunnel, and changes to `fprime-cli -> GDS -> OBC`.
