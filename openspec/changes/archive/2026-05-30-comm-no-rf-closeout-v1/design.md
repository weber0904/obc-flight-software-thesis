## Context

The repository now has all practical COMM slices needed for its current no-RF
simulation goal:

- the first exact target-bearing dual-link branch
- maintained per-band stock ground stacks plus a hosted-only thin lifecycle
  orchestration owner
- transport/APID governance
- bounded reliable transfer on default S-band node-`5`
- bounded reliable transfer on explicit-switched
  `uhf-primary-after-failover` node-`6`

The remaining drift is documentation and queue-shape drift. `docs/roadmap/`
still reads as if practical COMM completion is an active line item, and some
residual wording still blurs together:

- exact current baseline truth
- structural non-claims that must remain explicit
- optional future broadening
- out-of-scope items for the current repo goal

This change closes that gap without reopening any COMM runtime or operator
surface behavior.

## Goals / Non-Goals

**Goals:**

- remove COMM from the active practical queue for the repo's current no-RF
  simulation goal
- keep exact current COMM proofs and role/path semantics explicit
- preserve structural non-claims in the correct canonical layers
- classify remaining COMM topics consistently as:
  - dev-only residual
  - optional broadening
  - optional protocol residual
  - UX-only follow-up
  - structural non-claim
  - out-of-scope note
- archive the stale completed `comm-dual-link-orchestration-v1` workspace
  before closing out the new change

**Non-Goals:**

- no runtime behavior change
- no new verification-path claim
- no new hosted or target COMM probe
- no reopening of UHF packet quiet, UHF beacon suppress, target dual-link
  proof, transport MTU/APID governance, or reliable-transfer semantics
- no one-GDS / one-gateway design work
- no RF closure

## Decisions

### 1. Use one formal closeout change instead of ad hoc docs cleanup

The current drift affects canonical current-truth docs and OpenSpec workspace
state, so this closeout will be handled as a formal OpenSpec change rather
than a docs-only PR. That keeps the branch-verifiable rationale, tasks,
validation, and archive history explicit.

### 2. Use the narrowest possible spec delta

The change keeps product/runtime behavior unchanged. However, the spec-driven
schema still needs one formal capability delta. The narrowest honest fit is
`planning-docs`, because this change formalizes how a finished practical
capability line leaves the active roadmap queue while exact non-claims remain
in architecture/interface/current-baseline layers.

Alternatives considered:

- No spec delta at all
  - rejected because the active schema requires a `specs` artifact and this
    change genuinely adjusts roadmap-closeout expectations
- `documentation-governance`
  - rejected because the main issue is queue-shape and roadmap interpretation,
    not entrypoint freshness/checker mechanics

### 3. Keep planning demotion separate from non-claim retention

This is the main closeout rule:

- active queue items move out of `docs/roadmap/next-work.md`
- structural or proof-boundary non-claims remain in:
  - `docs/roadmap/current-baseline.md`
  - `docs/architecture/current-development-architecture.md`
  - `docs/interfaces.md`
  - `docs/architecture/comm-followup-directions.md` where useful

This prevents two opposite failures:

- overstating COMM completion by deleting needed non-claims
- understating COMM completion by leaving optional or out-of-scope items on the
  active queue

### 4. Clarify non-quiet UHF wording instead of preserving vague promotion language

Current exact truth already includes non-quiet `uhf-backup` as a bounded
backup/read-status/allowlisted command surface. The closeout should preserve
that exact truth and remove vague roadmap wording implying a generic
“non-quiet UHF promotion” is still waiting to make the repo practically usable.

The residual non-claim that remains is narrower: the repo does not broaden
every adjacent non-quiet UHF operator/observability story into a generic clean
nominal surface.

### 5. Archive the stale completed orchestration change first

`comm-dual-link-orchestration-v1` was already completed and synced, but its
OpenSpec workspace remained unarchived. This change will first archive that
workspace with `--skip-specs` because its delta specs were already synced to
main specs and reapplying them causes duplicate-header drift.

## Risks / Trade-offs

- **[Risk] Over-demotion could hide still-important structural boundaries**
  → Keep one clear split between active queue removal and retained non-claims.

- **[Risk] The roadmap could become empty or vague after COMM closeout**
  → Replace the active COMM row with a short optional broadening note and shift
  active priority language toward the already-documented non-COMM gaps.

- **[Risk] The stale orchestration workspace could leave OpenSpec in a noisy state**
  → Archive it first, then run reconciliation and consistency checks before
  finishing the new closeout change.

- **[Risk] The change could grow into a broad architecture rewrite**
  → Limit edits to wording that changes active queue status or clarifies
  category boundaries; do not restate unrelated subsystem gaps.
