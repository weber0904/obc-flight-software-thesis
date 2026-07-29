## Why

The repository has now grown well beyond the initial bootstrap follow-on queue, but the formal delivery documents still describe that original queue as if it were the full current roadmap. That drift makes it harder to review which changes completed the original baseline, which later capabilities were governed expansions, and which small governance or fix slices intentionally reused existing evidence instead of creating a new test-record directory.

## What Changes

- Add a repository-owned baseline reconciliation matrix that maps the original follow-on queue, the archived change history, the current main-spec set, and the evidence or exception trail for each archived change.
- Update the formal delivery workflow so the original follow-on queue is explicitly treated as the initial baseline queue rather than the full current change universe.
- Add a repo-local consistency checker that rejects placeholder main-spec purposes and missing archived-change coverage in the reconciliation matrix.
- Clarify the shared resource-storage baseline so later storage-oriented capabilities such as `storage-health` are described as governed extensions rather than accidental drift.
- Fix the stale placeholder Purpose text in the `scenario-driven-validation` main spec while keeping its requirement set intact.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `delivery-workflow`: add the baseline reconciliation matrix requirement, repo consistency checks, and initial-queue versus later-governed-expansion terminology.
- `resource-storage`: clarify that dedicated storage-oriented capabilities extend the shared storage baseline instead of redefining the storage roles independently.

## Impact

- Affected code: new repo-local consistency-check tooling and reconciliation data files.
- Affected systems: formal workflow governance, spec-review hygiene, archived-change traceability, and reviewer-facing documentation.
- No change to flight runtime behavior, simulator behavior, target integration behavior, or external command interfaces.
