## Why

The repository's current COMM baseline already closes the practical no-RF
simulation capability line: the exact target-bearing dual-link branch is
proven, the maintained hosted orchestration layers are in place, and bounded
reliable transfer now exists on both the default S-band node-`5` slice and the
explicit-switched `uhf-primary-after-failover` node-`6` slice. What remains is
mainly queue drift: current docs still read as if COMM is waiting on one more
practical step instead of treating the remaining items as residual non-claims,
optional broadening, UX polish, or out-of-scope notes.

This change is needed now because the active roadmap and current-facing docs
should stop presenting COMM as an unfinished practical queue for the repo's
current no-RF goal. Reviewers and later agents need one clean closeout slice
that preserves exact current proofs, removes stale active-queue wording, and
keeps formal non-claims explicit.

## What Changes

- Close out COMM as an active practical queue for the current no-RF simulation
  scope without changing runtime behavior or operator commands.
- Rewrite current roadmap/current-truth wording so COMM no longer appears as an
  active practical blocker.
- Demote remaining COMM items into explicit categories:
  - dev-only residual (`quiet rescue`)
  - optional broadening (`uhf-backup` reliable transfer)
  - optional protocol residuals (`restart-persistent resume`, broad CFDP)
  - UX-only follow-up (single integrated UI / one-console experience)
  - structural non-claims (`one-GDS`, `one-gateway`, broader simultaneous
    runtime arbitration, arbitrary file/downlink authority across all paths)
  - out-of-scope for the current repo goal (RF / over-the-air closure,
    automatic failover-to-UHF reliable transfer)
- Clarify that non-quiet `uhf-backup` bounded backup/read-status/allowlisted
  command behavior is already baseline truth and remove generic planning
  wording that implies the repo still needs a broad “non-quiet UHF promotion”
  to be practically complete.
- Archive the stale completed `comm-dual-link-orchestration-v1` workspace so
  the new closeout change starts from a clean OpenSpec state.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `planning-docs`: allow roadmap/current-baseline closeout of a finished
  practical capability line by replacing the former active queue item with
  optional broadening or out-of-scope notes while keeping exact formal
  non-claims in the canonical current-truth layers.

## Impact

- Affected docs:
  - roadmap/current-baseline/current-architecture/interface/follow-up wording
  - no operator workflow changes unless a runbook still misstates active COMM
    truth
- Affected OpenSpec workflow state:
  - archive the stale completed `comm-dual-link-orchestration-v1` workspace
  - add and archive one new documentation-governance closeout change
- No runtime, probe, gateway, GDS, or OBC behavior changes
