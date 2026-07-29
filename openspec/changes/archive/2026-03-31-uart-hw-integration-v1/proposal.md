## Why

The project already validates the external comm stack through TCP mock and PTY-backed virtual UART paths, and the Raspberry Pi target can build, package, and autostart successfully. Now that a Raspberry Pi to development-host UART/RS485 hardware path is available, the first real serial hardware slice should be integrated so the existing comm architecture can be exercised on physical links without redesigning the controller layer or changing the chosen UART-based protocol direction.

## What Changes

- Add a governed Raspberry Pi to host hardware UART validation path that reuses the existing `CommController`, `RadioController`, and `UartDriver` controller logic.
- Extend the hosted mock-radio backend and repo-local helper scripts so a development host can act as the hardware serial peer over a real tty device, not only TCP or PTY.
- Add a repo-local Raspberry Pi launch/probe flow for selecting an explicit target UART device path and exercising the real serial link end-to-end.
- Capture reviewable evidence for the first physical Raspberry Pi to host UART/RS485 exchange and update the remaining `Blocked-HW` boundary for this specific path.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: add a governed real-UART hardware validation path that preserves the shared controller-layer behavior while allowing a development host serial peer to back the physical link.
- `verification-evidence`: require reviewable evidence for Raspberry Pi to host hardware UART validation, including device selection, launch path, and observed serial-link behavior.

## Impact

- Affected code: `simulators/comm/`, `scripts/`, and possibly `OBC/Main.cpp` or related runtime helpers if transport naming/selection needs to be generalized from PTY-only wording to real serial devices.
- Affected docs: `README.md`, `obc-dev-spec/05_comm_subsystem.md`, `obc-dev-spec/07_verification_evidence.md`, and `docs/test-records/`.
- Affected systems: Raspberry Pi target serial device path selection, host-side serial peer launch, and real hardware comm validation evidence.
