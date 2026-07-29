## Context

The umbrella matrix suite already defines the environment taxonomy, nine case
ids, and summary outputs, but its current implementation intentionally leaves
some cells blocked and reuses historical probes with different granularity.
That is acceptable for a checkpoint, but the orchestration layer now needs a
clearer contract before additional follow-on changes start closing cells.

## Design

### Metadata Contract

Every case metadata record keeps the same required fields:

- `case_id`
- `environment`
- `carrier_kind`
- `registered_path_reuse`
- `new_claim_attempted`
- `verdict`
- `blocker_class`
- `rerun_safe`
- `artifact_root`
- `message`

The foundation change hardens the writer helpers so missing or malformed fields
become harness bugs instead of silent omissions.

### Case Intent Classification

Case wrappers are grouped into three explicit styles:

- direct reuse of an existing governed probe
- a narrowed wrapper around a larger governed probe
- a bounded blocker placeholder when no trustworthy harness exists yet

The summary output and wrapper comments must make that distinction obvious.

### Cleanup And Rerun Safety

The matrix cleanup smoke becomes the authoritative rerun-safety check for the
suite subtree. If touched orchestration logic fails cleanup smoke, later
follow-on changes do not proceed with new capability claims.

## Boundaries

- This change does not close blocked communication cells on its own.
- It does not widen any registry claim.
- It may refactor wrappers or helpers only enough to stabilize the matrix
  orchestration layer for later changes.
