# Design: payload-capture-modes-v2

## Summary

This change evolves the v1 payload contract into explicit capture families while
keeping `PayloadOpsController` as the only public owner. The active camera
session will carry a `PayloadSessionKind` of `AUTO`, `DETERMINISTIC`, or the
existing compatibility `LEGACY_V1` mapping used only while the PR is in
progress.

## Public Contract

- `PAYLOAD_PREPARE_SESSION(kind)`
- `PAYLOAD_SET_AUTO_DEFAULTS(...)`
- `PAYLOAD_SET_DETERMINISTIC_DEFAULTS(...)`
- `PAYLOAD_CAPTURE_AUTO(...)`
- `PAYLOAD_CAPTURE_DETERMINISTIC(...)`
- `PAYLOAD_GET_CAPABILITIES()`
- `PAYLOAD_GET_LAST_CAPTURE_METADATA()`

The v1 commands remain as wrappers for this PR stage:

- `PAYLOAD_PREPARE()` -> `PAYLOAD_PREPARE_SESSION(DETERMINISTIC)`
- `PAYLOAD_SET_DEFAULTS(...)` -> deterministic defaults wrapper
- `PAYLOAD_CAPTURE_STILL(...)` -> deterministic capture wrapper using the v1
  subset

## Configuration Model

- Default profiles are in-memory runtime truth and remain OFF-only.
- Per-capture override uses explicit apply flags instead of sentinel numeric
  values.
- Session-bound fields are rejected when provided as per-capture overrides.
- The first version of capability reporting will enumerate supported, OFF-only,
  and per-capture fields together with simple ranges/enums.

## Storage And Readback

Each successful capture writes:

- `capture-<boot>-<capture>.jpg`
- `capture-<boot>-<capture>.json`

The sidecar JSON is mission-truth metadata for requested and applied settings,
capture id, session kind, backend kind, and result/readback fields.

## Verification

- component UT for profile gating, override application, metadata caching, and
  legacy-wrapper behavior
- hosted probe using the canonical CCSDS node-5 path for auto/deterministic
  capture plus sidecar proof

## Sequencing Constraints That Shape This Change

- Official F´ sequencing is already active in `TopCcsds`, but the external
  operator contract is the repo-local wrapper surface
  `SEQ_VALIDATE/SEQ_RUN/SEQ_PREPARE_MANUAL/SEQ_START/SEQ_STEP/SEQ_CANCEL`, not
  direct stock `SeqDispatcher.RUN` or raw `CmdSequencer.CS_*`.
- Sequence files must be staged as `.sequence-staging/<leaf>`; absolute paths,
  parent traversal, and nested subdirectories are rejected before admission.
- `SequenceAdmissionController` inspects every inner opcode in the sequence
  before execution and rejects unknown opcodes, forbidden nested sequencing
  opcodes, and inner commands that fail current authority policy.
- Payload mutating commands stay primary/high-authority only. A sequence that
  contains `PAYLOAD_PREPARE_SESSION`, `PAYLOAD_CAPTURE_AUTO`, or
  `PAYLOAD_CAPTURE_DETERMINISTIC` must therefore be admitted from the primary
  command path, even though the wrapper `SEQ_*` commands themselves are allowed
  on backup ingress.
- `SequenceFileName` is bounded to `240` bytes and admitted internal command
  paths are still bounded by `Fw::CmdStringArg::STRING_SIZE`; the repo does not
  yet freeze a separate hosted S-band inner-payload ceiling beyond the current
  frame budget.
