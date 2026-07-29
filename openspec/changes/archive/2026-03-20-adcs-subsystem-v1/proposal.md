## Why

The repository now has a validated hosted subsystem pattern from EPS, but it still lacks the ADCS slice that exercises control-mode transitions, mission acceptance constants, and fault handling around attitude state. Implementing the first ADCS slice now gives the program a second transport-backed subsystem and validates that the simulator-to-bridge workflow scales beyond simple power state.

## What Changes

- Add the shared hosted ADCS request/response protocol definitions for node `3` and the ADCS service ports.
- Implement a hosted ADCS simulator with deterministic quaternion, angular-rate, pointing-error, and sensor-validity behavior over ZMQ request/response transport.
- Implement `AdcsBridge` as the OBC-owned F' component for the `ADCS_*` command, telemetry, and event families.
- Add focused `AdcsBridge` unit tests plus a host-side ZMQ integration test for the simulator/client path.
- Capture build and verification evidence for the ADCS subsystem implementation.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `adcs-subsystem`: add the concrete ADCS protocol, simulator, `AdcsBridge`, mission-constant-driven convergence behavior, and validated subsystem behavior for the first implementation slice.
- `verification-evidence`: add the evidence requirement for ADCS subsystem implementation and host-transport verification.

## Impact

- Affected code: `simulators/common/`, `simulators/adcs/`, `OBC/Components/AdcsBridge/`, `simulators/CMakeLists.txt`, `OBC/Components/CMakeLists.txt`
- Affected docs: `openspec/changes/adcs-subsystem-v1/`, `docs/test-records/`, `obc-dev-spec/04_adcs_subsystem.md`
- Dependencies: Homebrew `zeromq`, the project `fprime-venv/`, the F' normal build and UT build flows
