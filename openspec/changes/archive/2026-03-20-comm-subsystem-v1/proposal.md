## Why

The repository now has validated hosted subsystem slices for EPS and ADCS, but it still lacks the first implementation of the external communications path described by the formal narrative and baseline spec. Implementing the comm subsystem now establishes the reusable controller families, proves that TCP mock and PTY-backed UART-like paths can be exercised without hardware, and closes the first transport-switching verification gate required by the verification baseline.

## What Changes

- Add the first `CommController`, `RadioController`, and `UartDriver` components under `OBC/Components/`.
- Add a shared host-side byte-stream transport layer plus TCP mock and PTY-backed implementations under `simulators/comm/`.
- Add a simple hosted radio mock protocol and transport adapter so `RadioController` can be validated without hardware-specific framing.
- Add focused unit tests for the three comm components and a host integration test that exercises both TCP mock and PTY transport paths.
- Capture the implementation and verification evidence for the comm subsystem slice.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: add the first implementation slice for `CommController`, `RadioController`, `UartDriver`, and validated TCP/PTY transport switching behavior.
- `verification-evidence`: add the comm subsystem evidence requirement for TCP mock and PTY-backed UART replacement testing.

## Impact

- Affected code: `OBC/Components/`, `simulators/comm/`, `simulators/CMakeLists.txt`
- Affected docs: `openspec/changes/comm-subsystem-v1/`, `docs/test-records/`, `obc-dev-spec/05_comm_subsystem.md`, `simulators/README.md`
- Dependencies: project `fprime-venv/`, Homebrew `zeromq` already used by hosted tests, local loopback sockets, PTY support on the host OS
