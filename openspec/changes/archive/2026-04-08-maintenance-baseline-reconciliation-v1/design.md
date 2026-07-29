## Context

The repository now contains the original baseline implementation queue plus a second wave of governed expansions such as mission autonomy, scenario-driven validation, housekeeping archive, GPS, storage health, verification-path governance, and Codex skills. The current formal delivery documents still talk about the initial queue as if it were the repository's whole planned change set, and the current main-spec tree still contains one stale placeholder purpose. Reviewers therefore lack a single checked-in document that says which archived changes completed the original plan, which changes intentionally expanded the formal capability set, and which small governance or follow-up fixes reuse existing evidence rather than creating a new `docs/test-records/<change>/` directory.

## Goals / Non-Goals

**Goals:**
- create one reviewable, repository-owned reconciliation source that maps archived changes to capabilities and evidence or exception handling
- update the formal delivery workflow so the initial queue remains visible without pretending later governed expansions are unplanned drift
- add a local consistency check that catches placeholder purposes and unmapped archived changes before review
- clarify the formal relationship between `resource-storage` and later storage-oriented capabilities
- remove stale placeholder language from the `scenario-driven-validation` main spec

**Non-Goals:**
- changing any flight runtime behavior or target launch behavior
- introducing a coverage-report gate in this change
- rewriting historical archived evidence files unless needed to keep the reconciliation trail reviewable
- redefining verification-layer semantics that already live in `verification-evidence`

## Decisions

### Decision: Use a machine-readable matrix plus a human-readable summary

The reconciliation source should be checked in as both a human-readable markdown summary and a machine-readable JSON file. The markdown file is the review surface for humans, while the JSON file gives the consistency checker a stable source of truth for archived-change coverage, capability mapping, and evidence-exception handling.

Alternative considered:
- keep only a markdown narrative
  - rejected because the consistency checker would need fragile text parsing
- keep only a JSON file
  - rejected because reviewers should not have to inspect raw data to understand the repository history

### Decision: Treat missing `docs/test-records/<change>/` directories as explicit exceptions, not silent failures

Some archived changes are governance-only or small follow-up repairs that intentionally did not add a new evidence directory. The reconciliation matrix should therefore require each archived change either to cite one or more evidence paths or to declare an explicit reviewable exception with rationale.

Alternative considered:
- force every archived change to create a dedicated `docs/test-records/<change>/` directory retroactively
  - rejected because it would create artificial evidence for changes whose real output was formal workflow governance rather than feature verification

### Decision: Keep the new checker narrow and deterministic

The first consistency checker should verify a small set of repository truths that are both stable and high-value:
- no placeholder main-spec purposes
- every archived change with tasks is represented in the reconciliation matrix
- every listed capability exists in `openspec/specs/`
- every declared evidence path exists, or an explicit exception rationale is present

Alternative considered:
- make the checker validate every link and every requirement-level claim
  - rejected because it would turn this change into a much broader documentation linter and slow down adoption

### Decision: Reframe the follow-on queue as an initial baseline queue

The delivery workflow should retain the original queue because it explains how the repository bootstrapped, but it should no longer imply that later governed expansions are off-plan. The formal wording should distinguish the initial baseline queue from later archived expansions.

Alternative considered:
- delete the original queue entirely
  - rejected because the initial plan is still historically important and useful during maintenance review

## Risks / Trade-offs

- [Risk] The reconciliation matrix could become stale if later changes are archived without updating it. -> Mitigation: make the consistency checker fail when archived changes are missing from the matrix.
- [Risk] The matrix may duplicate some history already visible in archive directories. -> Mitigation: keep entries concise and focused on capability/evidence classification rather than retelling full implementation history.
- [Risk] Adding a new JSON source introduces another file to maintain. -> Mitigation: keep the schema minimal and document it in the markdown summary.
- [Risk] The resource-storage clarification could look redundant because `storage-health` already exists. -> Mitigation: make the added requirement explicitly about governed extension of shared storage roles, not about re-describing storage-health itself.

## Migration Plan

1. Add the reconciliation matrix JSON and markdown summary.
2. Add the narrow repo-local consistency checker and document how to run it.
3. Update `delivery-workflow` and `resource-storage` through delta specs, then sync them into the main specs.
4. Fix the stale placeholder purpose in `scenario-driven-validation`.
5. Add an evidence record for this maintenance slice, run the checker plus OpenSpec validation, archive the change, and commit it.
