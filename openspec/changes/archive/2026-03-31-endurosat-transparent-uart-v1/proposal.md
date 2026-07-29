## Why

The current host-side mock transceiver is sufficient for validating the existing command/status-oriented comm baseline, but it does not model the legacy EnduroSat S-band transceiver's UART transparent mode. The project now has a verified Raspberry Pi to host UART/RS485 path, so the next step is to validate a data-plane-first integration path that is closer to the real legacy radio behavior without prematurely redesigning the full comm stack.

## What Changes

- Add a governed legacy EnduroSat transparent-UART validation path that focuses on raw payload transport over the existing serial link.
- Introduce a host-side transparent peer mode or companion simulator that behaves like a transparent serial peer instead of the current text command/status mock.
- Extend repo-local launch and probe scripts so Raspberry Pi and host runs can select the transparent validation flow, including the legacy UART baudrate.
- Record explicit evidence for what this change validates and what remains out of scope, especially ESPS control/configuration behavior and newer `csp-es`-based hardware generations.

## Capabilities

### New Capabilities

<!-- None -->

### Modified Capabilities

- `comm-subsystem`: add a governed transparent-UART data-path validation mode for legacy EnduroSat hardware while keeping the existing controller-layer comm baseline intact.
- `verification-evidence`: require reviewable evidence for the legacy EnduroSat transparent-UART validation flow and explicit scope boundaries for the still-missing control plane.

## Impact

- Affected code: `simulators/comm/`, `scripts/`, and comm runtime wiring used by Raspberry Pi UART validation.
- Affected behavior: adds a second governed hardware-validation path that is closer to legacy EnduroSat transparent mode than the current `mock-text` peer.
- Dependencies: no new mandatory external library is expected; the change reuses the existing serial link and Raspberry Pi validation path.
- Systems: macOS host peer tooling, Raspberry Pi target runtime, and comm verification evidence.
