## Context

The repository now has a bounded physical lab serial COMM TT&C proof for command ingress, command events, and telemetry through:

```text
fprime-cli -> GDS -> ground_ttc_gateway -> physical serial
  -> subsystem.local comm_csp_node -> CSP -> hosted OBC
  -> COMM downlink -> ground_ttc_gateway -> GDS -> fprime-cli
```

That evidence deliberately excluded file/downlink. Separately, `HousekeepingArchive` already exposes `HK_DOWNLINK_INDEX` and `HK_DOWNLINK_SLOT` commands that queue files through F' `FileDownlink`. This change combines those existing surfaces only for proof, not by adding a new file command or changing COMM CSP services.

## Goals / Non-Goals

**Goals:**

- Prove existing housekeeping archive file/downlink over the COMM TT&C path.
- Keep hosted PTY COMM file/downlink as a regression before the physical verdict.
- Require at least two occupied housekeeping archive slots and verify two downlinked slot files byte-for-byte.
- Preserve command/event/channel TT&C as a prerequisite in the physical probe.
- Keep all runtime roots, GDS downlink directories, ports, and physical endpoints explicit in evidence.

**Non-Goals:**

- No generic arbitrary file downlink command.
- No COMM CSP service-port or wire-layout change.
- No archive policy runtime knob; slot generation uses paced `HK_CAPTURE_NOW`.
- No RF, real radio, target OBC migration, no-preamble first-byte-clean claim, ScenarioBridge/pass automation, or COMM shared CAN FD participation.

## Decisions

- Use existing `HK_*` command surface.
  - Rationale: it is already governed and bounded to archive index/slot files.
  - Alternative rejected: generic file command, because that would add access-control and operator-surface scope unrelated to proving COMM file transport.
- Generate two slots with paced captures.
  - Rationale: this proves multiple slot file transfers without adding product knobs or running a full ring/wraparound proof.
  - Alternative rejected: natural cadence, because it is too slow for a repository-owned probe.
- Add distinct hosted and physical probe entrypoints.
  - Rationale: hosted PTY regression catches COMM/GDS file path regressions before hardware timing enters the verdict.
  - Alternative rejected: extending the command/event/channel probe in place, because it would blur the proof boundary and evidence naming.
- Validate by ground-file existence and byte-for-byte comparison.
  - Rationale: events alone only prove a queued transfer; file/downlink proof needs the received files and exact source match.

## Risks / Trade-offs

- [Risk] Many `HK_CAPTURE_NOW` commands can pressure the command queue.
  - Mitigation: pace commands, poll the runtime index, and stop once two slots are occupied.
- [Risk] FileDownlink may complete after command acknowledgement.
  - Mitigation: poll the GDS file-storage directory with bounded timeouts before comparing files.
- [Risk] Physical serial timing can hide whether a failure is file-specific or TT&C-specific.
  - Mitigation: require the existing command/event/channel prerequisite before file assertions.
- [Risk] Multi-slot proof could be mistaken for full archive policy proof.
  - Mitigation: evidence states this is at least two occupied slots, not full ring fill or generation wraparound.
