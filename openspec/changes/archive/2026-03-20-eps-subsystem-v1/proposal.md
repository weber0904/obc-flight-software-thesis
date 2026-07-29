## Why

The repository now has a bootstrapped F' workspace and shared core contracts, but it still lacks the first real subsystem that exercises the intended simulator-to-bridge workflow. Implementing the EPS slice now establishes the first transport-backed subsystem path, validates the hosted simulator pattern, and gives later ADCS/COMM/boot work a concrete example to build on.

## What Changes

- Add the shared EPS request/response protocol definitions for node `2` and service ports `1` through `4`.
- Implement a hosted EPS simulator with ZMQ request/response transport and a deterministic battery/solar/PDU state model.
- Implement `EpsBridge` as the OBC-owned F' component for the `EPS_*` command, telemetry, and event families.
- Add focused bridge unit tests plus a host-side ZMQ integration test for the simulator/client path.
- Capture build and verification evidence for the EPS subsystem implementation.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `eps-subsystem`: add the concrete EPS protocol, simulator, `EpsBridge`, and validated subsystem behavior for the first implementation slice.
- `verification-evidence`: add the evidence requirement for EPS subsystem implementation and host-transport verification.

## Impact

- Affected code: `simulators/common/`, `simulators/eps/`, `OBC/Components/EpsBridge/`, root `CMakeLists.txt`
- Affected docs: `openspec/changes/eps-subsystem-v1/`, `evidence/records/`, `obc-dev-spec/03_eps_subsystem.md`
- Dependencies: Homebrew `zeromq`, the project `fprime-venv/`, the F' normal build and UT build flows
