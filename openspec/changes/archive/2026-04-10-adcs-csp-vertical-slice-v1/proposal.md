## Why

The libcsp-first recovery base has established the hosted internal CSP substrate and migrated EPS business traffic, but ADCS remains on the legacy project-local ZMQ request/response path. This change completes the next vertical slice by moving hosted ADCS business traffic to libcsp node `3` while preserving the existing F' `AdcsBridge` command, telemetry, event, mission, and housekeeping contracts.

## What Changes

- Convert the hosted ADCS simulator from a plain ZMQ request/response server into a libcsp service node on formal node ID `3`.
- Add ADCS-owned CSP request/reply payload definitions and service ports for state, mode, target, and calibration traffic.
- Add `CspAdcsTransport` and make the default `AdcsBridge` transport use the internal CSP runtime instead of the legacy ZMQ transport.
- Keep the existing `AdcsBridge` F' public behavior, scheduler polling, cached-state behavior, convergence signaling, and failure handling semantics.
- Add a hosted ADCS CSP integration smoke proving node `1` to node `3` business traffic over the ZMQHUB-backed internal CSP substrate.
- Update verification registry, matrix/evidence, and ADCS formal/narrative specs so the legacy direct-ZMQ ADCS path is no longer described as the future architecture baseline.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `adcs-subsystem`: Replace the hosted ADCS shared ZMQ request/response requirement with an ADCS-owned CSP service model on node `3`.
- `verification-evidence`: Register the hosted ADCS internal libcsp service path and its evidence boundaries.
- `verification-path-registry`: Add the ADCS CSP vertical-slice path and explicitly exclude ground, external comm, GPS, and real hardware paths.

## Impact

- Affected code: `simulators/adcs/*`, `OBC/Components/AdcsBridge/*`, `simulators/CMakeLists.txt`, and hosted stack scripts if they still pass legacy ADCS endpoint assumptions.
- Affected tests: ADCS transport integration tests, `AdcsBridge` component UT, CSP runtime smoke, and the verification inventory/reporting paths.
- Affected docs/evidence: `openspec/specs/adcs-subsystem`, verification specs/registry, `obc-dev-spec/04_adcs_subsystem.md`, and `docs/test-records/adcs-csp-vertical-slice-v1/`.
- Exclusions: no changes to ground GDS path, external comm path, GPS path, real hardware bring-up, scheduler/payload operations, or ADCS flight-control semantics beyond moving the hosted internal transport substrate.
