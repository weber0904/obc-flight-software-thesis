# Design: payload-sensor-register-controls-v1

## Summary

This change adds governed raw sensor register controls without redefining the
payload contract as a generic debug transport. Raw register commands are valid
only after `PAYLOAD_PREPARE_SESSION(RAW_SENSOR)`.

## Public Contract

- `PAYLOAD_SENSOR_REG_READ(addr)`
- `PAYLOAD_SENSOR_REG_WRITE(addr, value, verifyReadback)`
- `PAYLOAD_CAPTURE_RAW(...)`

`PAYLOAD_SENSOR_REG_WRITE_SEQUENCE(sequenceId)` is deferred unless repeated
single writes prove insufficient for official sequence use.

## Boundaries

- only valid in `PAYLOAD` mode
- only valid in prepared `RAW_SENSOR` session
- only valid on primary/high-authority ingress surfaces
- hosted uses a fake adapter and states that it does not prove real register
  effects

## Verification

- unit tests for mode/session/authority gating
- hosted fake proof on the CCSDS node-5 path
- target proof for at least one real read/write path if the target kernel and
  userspace stack expose a maintainable interface

## Sequencing Constraints That Shape This Change

- RAW sensor register operations run through the same governed official
  sequencing wrapper surface as the rest of the OBC: `SEQ_VALIDATE/SEQ_RUN`
  through `SequenceAdmissionController`, not direct stock `CmdSequencer`
  controls.
- Sequence files still must use the governed `.sequence-staging/<leaf>` logical
  destination; this change does not broaden file-ingress scope.
- `SequenceAdmissionController` performs inner-opcode inspection and authority
  checks before execution. `PAYLOAD_SENSOR_REG_WRITE` is primary/high-authority
  only, so sequences containing sensor-register writes must be admitted from the
  primary command path. `PAYLOAD_SENSOR_REG_READ` may remain backup-readable
  outside sequences as a bounded read/status surface.
- Nested sequencing/admin opcodes remain forbidden inside payload register
  sequences, so this change does not rely on embedding stock `CS_*`/dispatcher
  controls in raw-register workflows.
