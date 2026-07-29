## Why

The repository already has a governed shared CAN FD-capable internal CSP baseline for `OBC`, `EPS`, and `ADCS`, but `COMM` still sits outside that subsystem network as a controller-oriented external link path. At the same time, the formal architecture has already chosen a separate omitted-RF TT&C direction where ground-originated traffic should reach OBC through a spacecraft-side comm subsystem instead of overloading the existing direct `GDS -> TCP -> OBC` development path.

This change is needed now to turn `COMM` into a real CSP-facing subsystem and to add the first repository-owned ground gateway path, while keeping the current direct GDS and external comm baselines alive and explicitly distinct.

## What Changes

- Add a governed `COMM` CSP identity with node `4` and a reserved COMM-owned service-port block `30-39`.
- Introduce the first `COMM` CSP protocol and subsystem-side node that bridges lab-side serial ingress to the shared internal CSP bus.
- Add an OBC-side `COMM` integration boundary that can accept bounded uplink command traffic from `COMM` and return bounded event/telemetry downlink without collapsing direct GDS and external comm architecture domains together.
- Add a repository-owned ground gateway process that sits between stock `fprime-gds` tooling and the lab-side `COMM` ingress while reusing stock F' framing.
- Add focused tests, governed probes, registry entries, and evidence for the first bidirectional omitted-RF `command/event/tlm` path through `COMM`.

## Capabilities

### New Capabilities
- none

### Modified Capabilities
- `comm-subsystem`: define the first governed COMM CSP-facing subsystem path while preserving current mock/UART external comm baselines as separate evidence.
- `core-system-contracts`: formalize `COMM` node `4`, reserved COMM service-port ownership, and the required separation between COMM TT&C, direct GDS, and external comm domains.
- `ground-ttc-gateway`: narrow the first gateway-backed scope to a repository-owned GDS-facing adapter for bounded bidirectional `command/event/tlm`.
- `verification-path-registry`: register the new gateway-backed COMM TT&C path separately from the direct `GDS -> TCP -> OBC` and historical/current external comm paths.
- `verification-evidence`: require reviewable test evidence for the first gateway-backed bidirectional omitted-RF TT&C flow and its baseline-reuse boundaries.

## Impact

- Affected code:
  - `OBC/Components/` and `OBC/Top/` for the new COMM CSP integration boundary
  - `simulators/comm/` and `simulators/csp/` for the COMM node, protocol, and gateway bridge pieces
  - `scripts/` for governed gateway/probe launchers
  - `docs/test-records/` and `docs/verification-path-registry.md` for new evidence
- Affected interfaces:
  - new COMM CSP protocol/service definitions
  - new or extended CSP runtime abstractions for COMM-facing traffic handling
  - new governed gateway/probe scripts and environment variables
- Affected systems:
  - shared CAN FD internal CSP path now includes `COMM`
  - direct `GDS -> TCP -> OBC` development baseline remains unchanged
  - controller-oriented external comm remains a separate bounded baseline rather than becoming the TT&C end-state
