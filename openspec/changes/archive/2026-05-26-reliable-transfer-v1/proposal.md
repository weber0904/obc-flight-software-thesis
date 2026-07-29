## Why

The current default S-band node-`5` baseline can already prove official `.fdp`
file/downlink over the governed COMM path, but it still uses stock
`DpCatalog -> FileDownlink -> COMM` semantics: once a file/downlink attempt is
accepted, delivery truth is still "whole file succeeded or ground retries the
whole command again later". That leaves a review gap between:

- ground-side whole-command retry before or after a transfer attempt, and
- bounded transport/file-delivery semantics inside a single transfer attempt.

This change introduces the first bounded reliable-transfer slice on the
existing stable baseline without reopening broader COMM policy, gateway role,
`SESSION_OPEN(seq0)`, observability ownership, RF closure, UHF redesign, or
generic all-path file authority work.

The slice is intentionally narrow:

- current default S-band node-`5` path only
- existing official HK `.fdp` artifact family only
- one active transfer at a time
- fixed-size data segments with bounded cumulative-ACK resend
- hosted proof first, plus the existing target/lab default node-`5` path

## What Changes

- Add a repo-local reliable-transfer sidecar path for selected official HK
  `.fdp` transfers on the current default S-band node-`5` COMM baseline.
- Keep `DpCatalog` as the owner of HK `.fdp` generation/catalog selection and
  keep `CommController` as the shared downlink policy owner.
- Add a bounded `CommController` helper that performs file-transfer state,
  transfer start, fixed-size segment send, cumulative ACK polling, timeout
  resend, bounded retry exhaustion, and final completion truth.
- Align the new path with F´ `Fw::FilePacket` lifecycle vocabulary:
  `START/DATA/END/CANCEL`.
- Add narrow COMM sidecar services for reliable-transfer data and control on
  node `5`.
- Add a repo-owned node-`5` ground-side receiver on the COMM simulator side so
  reliable transfer truth is not delegated to stock GDS file-store behavior.
- Add repository-owned probes for happy path, degraded ACK/timeout resend, and
  bounded retry-exhausted failure.

## Scope Boundaries

This change does:

- define the first bounded reliable-transfer semantics for one current artifact
  family on one current path
- separate ground whole-command retry from in-transfer reliability semantics
- prove success, timeout, resend, duplicate-in-context, cancel/abort, and
  partial-progress final failure on the repo-owned path

This change does not:

- redesign all COMM paths
- adopt a generic CFDP platform
- change UHF runtime behavior
- add simultaneous dual-link orchestration
- prove RF over-the-air behavior
- redesign generic file ownership or mission planning
- reopen gateway role, accepted `SESSION_OPEN(seq0)`, or broader COMM policy

## Upstream Context

F´ official current file handling provides `FileDownlink`, `FileUplink`, and
`DpCatalog`, but not a current built-in reliable-transfer runtime or current
CFDP delivery engine. The official `DpCatalog` design documentation still
describes whole-file delivery where an interrupted downlink requires the whole
file to be sent again, and the F´ roadmap still keeps CFDP support in future
work. This change therefore remains repo-local and bounded instead of claiming
upstream CFDP adoption.
