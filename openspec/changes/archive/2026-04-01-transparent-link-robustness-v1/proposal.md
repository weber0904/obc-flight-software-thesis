## Why

The current transparent link framing path proves that framed binary payloads can cross the Raspberry Pi to host UART/RS485 path, but it only covers short governed exchanges with a stable peer. Before this path can support later ground-gateway or vendor-adapter work, the repository needs governed evidence that the framed link remains usable during sustained exchange and after disconnect or restart events.

## What Changes

- Add governed robustness coverage for the framed transparent UART path, including repeated binary-safe frame exchange over the existing Raspberry Pi to host serial link.
- Add a governed disconnect/reconnect validation flow that proves the framed path can recover after the host-side transparent peer is interrupted and restarted.
- Extend the host-side transparent peer and probe tooling so robustness scenarios can be exercised without replacing the current direct TCP/GDS baseline or the current `mock-text` radio baseline.
- Record dedicated evidence for sustained framed exchange and reconnect recovery, and keep the remaining ground-gateway, RF, and vendor-control work explicitly out of scope.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: extend the framed transparent UART path requirements to cover repeated framed exchange and recovery after host-peer disconnect or restart.
- `verification-evidence`: require reviewable evidence for sustained framed exchange and framed-link reconnect recovery.

## Impact

- Affected code: `simulators/comm`, `scripts/`, `OBC/Main.cpp`, and transport/integration tests for the comm stack.
- Affected systems: Raspberry Pi to host UART/RS485 governed validation flow, host-side transparent peer modes, and comm evidence/docs.
- No breaking changes to the current TCP/GDS baseline, `mock-text` baseline, or the current transparent frame v1 format.
