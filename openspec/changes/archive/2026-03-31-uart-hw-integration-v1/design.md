## Context

The external comm stack is already structured so transport-specific details sit below `CommController`, `RadioController`, and `UartDriver`. Hosted validation currently covers TCP mock and PTY-backed UART replacement, while the Raspberry Pi target profile can already select an explicit `--comm-device` path for the serial side. What is missing is a governed real-hardware validation slice: the development host mock-radio backend still only binds TCP or PTY, the repo-local scripts do not expose a first-class hardware UART flow, and the evidence tree still leaves this path under `Blocked-HW`.

The user has now confirmed a Raspberry Pi ↔ macOS serial path works over the available hardware chain (`macOS -> UT-890A -> RS485-to-TTL -> Raspberry Pi`). The design goal is therefore to preserve the current UART-based architecture and add the smallest set of code and workflow changes needed to exercise it against a real serial peer.

## Goals / Non-Goals

**Goals:**
- validate the existing UART-based external comm architecture over a real Raspberry Pi to host serial link
- keep controller-layer logic unchanged and continue using the shared byte-stream abstraction
- let the host-side mock-radio backend attach to a real serial device path, not only TCP or PTY
- provide repo-local scripts for launching the host serial peer and Raspberry Pi hardware-UART stack
- capture reviewable evidence that clears the specific Raspberry Pi ↔ host UART `Blocked-HW` item

**Non-Goals:**
- changing the selected external comm architecture from UART to SPI, I2C, CAN, or another bus
- adding KISS or vendor-specific radio framing in this change
- claiming full real-radio, GPIO, or I2C power-sequencing validation
- removing existing TCP mock or PTY validation flows

## Decisions

### 1. Preserve the current UART/serial controller architecture

This change will explicitly keep the current `CommController` / `RadioController` / `UartDriver` layering and continue to treat the external link as a byte-stream transport. The hardware slice is a validation of the chosen architecture, not a redesign.

Alternatives considered:
- switching to CAN before hardware validation: rejected because it would change both bus semantics and implementation scope before the current UART architecture is fully exercised
- switching to I2C or SPI: rejected because those buses do not match the current byte-stream controller design and would force broader refactoring

### 2. Extend the hosted mock-radio backend to open a real serial device path

The existing `radio_mock_server` will be extended with an explicit serial-device mode so the macOS development host can act as the hardware serial peer over `/dev/tty.*` or `/dev/cu.*` device paths. This preserves one mock-radio protocol implementation across TCP, PTY, and hardware-serial modes.

Alternatives considered:
- writing a separate one-off host serial echo tool: rejected because it would duplicate the mock-radio protocol logic and split future maintenance
- requiring manual terminal interaction on the host side: rejected because it would not give a governed, repeatable validation path

### 3. Generalize user-facing wording from PTY-only to serial-device selection

The runtime and helper scripts currently use `pty` wording even though the Raspberry Pi side transport already accepts a generic tty device path. This change will generalize user-facing terminology and helper script options so the same path can be used for PTY-backed testing or real hardware UART devices without implying that only pseudo-terminals are supported.

Alternatives considered:
- keeping the `pty` label while silently using real tty devices: rejected because it would make the hardware validation flow harder to understand and document

### 4. Add repo-local launch/probe scripts for the hardware-UART slice

The governed flow will include one host-side helper for launching the mock-radio backend against a serial device path and one Raspberry Pi helper for running the target stack against a selected UART device. A short probe script will exercise the end-to-end path and capture deterministic operator commands.

Alternatives considered:
- relying only on raw ad hoc SSH and host-terminal commands: rejected because the resulting evidence would be less repeatable and less reviewable

## Risks / Trade-offs

- [Host and target serial device names can differ across reconnects] → Keep device paths as explicit helper inputs or environment variables rather than hard-coding them in controller logic.
- [Real serial wiring may still expose RS485/TTL electrical edge cases outside software control] → Limit this change to the validated hardware chain already proven to transmit successfully, and leave broader hardware permutations out of scope.
- [Extending the mock-radio backend to new transport modes could accidentally regress TCP or PTY workflows] → Preserve existing modes, add focused tests where possible, and rerun the existing shared comm verification gate.
- [One successful Raspberry Pi ↔ host UART path could be misread as full radio integration] → State clearly in specs and evidence that this change validates the real serial link with the hosted mock peer, not a real radio or vendor protocol.

## Migration Plan

1. Add a serial-device mode to the hosted mock-radio backend and update runtime/help text to reflect generic serial-device support.
2. Add repo-local host/target helper scripts for the real UART hardware path.
3. Run the existing local verification gate to confirm TCP and PTY paths still work.
4. Launch the host serial peer and Raspberry Pi hardware-UART stack against the known-good device paths.
5. Record evidence, validate the change, and archive it.

## Open Questions

- Whether the macOS host helper should prefer `/dev/cu.*` or `/dev/tty.*` naming by default when both are present.
- Whether a later change should teach the installed Raspberry Pi release/service flow to run directly against the hardware UART path instead of the current TCP mock baseline.
