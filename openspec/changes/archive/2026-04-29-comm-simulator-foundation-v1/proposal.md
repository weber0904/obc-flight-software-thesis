## Why

The repository now has a governed gateway-backed omitted-RF `COMM` path, but the hosted `COMM` stand-in still mixes CSP service handling, serial I/O, queue state, and simulator behavior inside `CommNodeServer`. This change is needed to give `COMM` the same simulator foundation shape as `EPS` and `ADCS`: a CSP node/server shell backed by an explicit business model.

## What Changes

- Add a `CommSimModel` business layer for hosted `COMM` simulator state and behavior.
- Move link state, bounded uplink queue behavior, downlink acceptance/backpressure, status counters, overflow tracking, and model-only disconnect/error injection semantics out of the server shell.
- Keep `CommNodeServer` and `comm_csp_node` as the runtime CSP/serial shell, preserving the existing gateway-backed TT&C behavior.
- Preserve the current CSP wire contract: `COMM` node `4`, service range `30-39`, and existing services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`.
- Add focused model unit coverage and rerun the governed gateway-backed probe to prove the previous path remains compatible.

## Capabilities

### New Capabilities
- none

### Modified Capabilities
- `comm-subsystem`: require the hosted `COMM` CSP stand-in to use a simulator model/server split while preserving the existing gateway-backed CSP wire contract and adjacent-path boundaries.

## Impact

- Affected code:
  - `simulators/comm/` for the new model, server refactor, and tests
  - `simulators/CMakeLists.txt` for model build/test registration
  - `docs/test-records/` for focused evidence
- Affected interfaces:
  - new C++ model-facing `CommSimConfig` and `CommSimStatus` types
  - no change to the existing CSP packet sizes, services, node id, or service-port allocation
- Affected systems:
  - gateway-backed omitted-RF path remains the focused compatibility probe
  - direct `GDS -> TCP -> OBC`, controller-oriented external comm, RF, file/downlink, custom GDS plugin, and COMM SocketCAN proof stay out of scope
