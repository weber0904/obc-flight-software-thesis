## Context

The repository already enforces a baseline verification gate and preserves detailed evidence under `docs/test-records/`, but the current project-wide answer to "what is actually covered?" still depends on manual repo reading. Earlier controller-style components use classic F' component UT harnesses, while later slices such as GPS, storage health, and housekeeping archive rely more on focused unit tests, integration tests, and hosted probes. That mix is not wrong, but it needs one governed review surface so future maintenance can prioritize real gaps instead of guessing.

## Goals / Non-Goals

**Goals:**
- add one checked-in verification matrix that answers L1/L2/L3/L4 coverage capability-by-capability
- add a repo-local inventory tool that derives registered tests, probes, and evidence directories from the current repository
- highlight the current weak spots that should drive the next UT-backfill slice

**Non-Goals:**
- adding line or branch coverage tooling in this change
- rewriting the existing tests or probes themselves
- forcing every newer slice back into a classic F' harness pattern
- replacing the detailed change-level evidence records in `docs/test-records/`

## Decisions

### Decision: Keep the inventory auto-derived but keep the matrix human-curated

The inventory tool should scan the repo for registered tests, probe scripts, and evidence directories, while the checked-in matrix should remain a human-reviewed capability summary. The tool provides current truth; the matrix adds interpretation about gaps and constrained validation.

Alternative considered:
- derive the whole matrix automatically
  - rejected because capability-level gap statements still need human review

### Decision: Classify coverage by layer and by review surface

The matrix should list L1, L2, L3, and L4 separately, and it should allow a capability to be strong in L4 while still weak in L2. This prevents the matrix from pretending that all tests are interchangeable.

Alternative considered:
- collapse everything into one "tested / not tested" column
  - rejected because it would hide the exact problem this maintenance slice is trying to surface

### Decision: Record weak spots explicitly in the matrix

The matrix should include explicit notes for areas such as `GpsBridge`, `StorageHealthBridge`, `HousekeepingArchive`, `HousekeepingSnapshotProvider`, and transparent framing/protocol-adapter paths where coverage style is known to be uneven.

Alternative considered:
- leave weak spots to future narrative discussion
  - rejected because the next UT-backfill change needs a governed starting point

## Risks / Trade-offs

- [Risk] The matrix may become stale as new tests are added. -> Mitigation: keep the inventory tool repo-local and rerunnable so later changes can refresh the review surface.
- [Risk] The matrix may overstate certainty if it is too optimistic. -> Mitigation: include constrained gaps and weak-spot notes explicitly.
- [Risk] The inventory tool may not understand every future test registration pattern. -> Mitigation: keep the first version focused on the patterns the repository uses today.

## Migration Plan

1. Add the verification-evidence delta spec for the matrix and inventory tool.
2. Add the repo-local inventory script and the checked-in verification matrix.
3. Update docs indexes to point reviewers at the new matrix.
4. Add evidence capturing the inventory output and validation results, then archive and commit.
