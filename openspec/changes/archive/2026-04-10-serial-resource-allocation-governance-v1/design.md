## Context

The repository has a working Raspberry Pi external comm UART path that uses `/dev/serial0`. The GPS subsystem is intentionally a separate subsystem boundary, but its future live transport may also be UART. Those facts must be reconciled before live GPS hardware work begins.

## Decisions

1. **Reserve `/dev/serial0` for external comm/radio in the current baseline.**
   - This preserves the existing governed Pi comm evidence.
   - GPS live UART cannot silently reuse that device.

2. **Defer GPS live UART until a non-conflicting allocation exists.**
   - Preferred future test path is a USB-UART device with stable `/dev/serial/by-id/...` naming.
   - Secondary Pi UART overlays or pin mux changes require a later hardware architecture decision.

3. **Do not time-multiplex comm and GPS on one UART.**
   - GPS NMEA and external radio sessions have different traffic assumptions.
   - Sharing one UART would be brittle and not representative without additional switching hardware and governance.

## Risks / Mitigations

- **Risk:** GPS live work is blocked longer than desired.
  - **Mitigation:** Keep hosted GPS fake/replay as the validated baseline and define USB-UART as the least disruptive future bring-up path.
- **Risk:** Documentation implies `/dev/serial0` is generic serial capacity.
  - **Mitigation:** README, scripts README, matrix, and specs call out the ownership explicitly.
