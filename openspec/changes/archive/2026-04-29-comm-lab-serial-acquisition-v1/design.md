## Context

The prior UART preflight proved macOS-initiated request/reply only. A guarded full TT&C attempt over the physical lab serial link then showed that the end-to-end path is not ready: OBC saw COMM ground-link traffic and errors, but the bounded EPS/ADCS commands were not reflected in OBC readback. This points to the serial middle segment, especially acquisition/turnaround on the physical RS-485 path, rather than to a missing F' command definition.

## Goals / Non-Goals

**Goals:**

- Prove a smaller, reviewable `subsystem.local -> macOS` passive-receiver acquisition path.
- Use a deterministic preamble plus framed payload format so the receiver can ignore cold-start garbage and resynchronize.
- Keep the probe independent of GDS, OBC, `ground_ttc_gateway`, and COMM CSP services.
- Preserve enough diagnostic context to guide the next TT&C retry.

**Non-Goals:**

- No full TT&C PASS claim.
- No changes to `ground_ttc_gateway`, F' framing, COMM node services, RF, file/downlink, ScenarioBridge, target OBC, cross-compile, or COMM CAN.

## Decisions

1. Use standard-library Python raw serial helpers inside a repo-owned shell probe.

   Rationale: this avoids adding `pyserial` as a repository dependency while still letting the probe set raw termios mode, capture bytes, and parse bounded frames on macOS and Linux.

2. Use an acquisition frame instead of plain text lines.

   Rationale: prior manual `screen` tests showed readable text can appear after garbage, but a future gateway cannot rely on operator-visible text. A magic + length + sequence + CRC frame lets the receiver scan through garbage and prove exact payload recovery.

3. Keep the proof below TT&C.

   Rationale: even a PASS here only proves a bounded subsystem-origin serial acquisition strategy. It does not prove that stock F' frames can survive the current half-duplex traffic pattern.

## Risks / Trade-offs

- The acquisition probe may pass while full TT&C still fails under simultaneous bidirectional load -> record that limitation explicitly and leave TT&C for a later retry.
- The probe may fail if the RS-485 adapter direction control or line state is unstable -> evidence should preserve raw byte counts and any decoded-frame count for hardware debugging.
- Python termios behavior differs slightly across macOS/Linux -> keep baudrates to common constants and fail early for unsupported values.
