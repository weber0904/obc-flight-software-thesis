## Why

The COMM lab serial ingress work depends on a physical UART link that has moved from the OBC-side Raspberry Pi to `subsystem.local`. Before routing TT&C through that link, the repository needs a narrow preflight proof that macOS can initiate bounded request/reply traffic into `subsystem.local` over the new physical serial path.

## What Changes

- Add a serial probe utility that opens an explicit requester serial device and performs bounded `mock-text` exchanges.
- Add a repository-owned host probe script that starts a native-built `radio_mock_server` peer on `subsystem.local`, runs the macOS-side requester against the host serial device, captures logs, and asserts the final `STATUS enabled=1` result.
- Preserve the current `subsystem.local` native workspace build baseline; do not add Docker, cross-compilation, sysroot, or installed subsystem bundles in this change.
- Refresh stale reviewer-facing documentation that still describes the hosted gateway-backed omitted-RF TT&C path as future-only.
- Record evidence that separates this physical UART preflight from clean subsystem-origin cold-first downlink, TT&C, RF, file/downlink, direct TCP, target OBC, and COMM shared CAN FD behavior.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: add a bounded macOS-initiated subsystem-side COMM UART preflight validation path for the new macOS-to-`subsystem.local` serial wiring.
- `verification-evidence`: require reviewable evidence for the preflight probe endpoints, logs, observed exchange, constrained scope, and reverse-direction diagnostic limitations.
- `verification-path-registry`: register the subsystem-side COMM UART preflight path separately from historical OBC-side UART comm evidence and from gateway-backed TT&C.

## Impact

- Affected code: simulator COMM helper executable, simulator CMake registration, repository probe scripts.
- Affected docs: `README.md`, `docs/verification-matrix.md`, `evidence/verification-path-registry.md`, and `evidence/records/subsystem-comm-uart-link-preflight-v1/`.
- Affected systems: macOS host serial endpoint and `subsystem.local` serial endpoint only.
