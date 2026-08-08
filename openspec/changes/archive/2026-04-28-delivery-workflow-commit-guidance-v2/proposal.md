## Why

The previous attempt to clarify rollback commits and first-push commit boundaries became entangled with review churn and an eventual revert, leaving the repository with an incomplete audit trail. The workflow rule needs a clean replacement from the reverted baseline, plus an explicit correction to the reconciliation record for the superseded attempt.

## What Changes

- Add a clean formal `delivery-workflow` requirement that separates:
  - local rollback commits during complex development
  - the required single Conventional Commit before the first push / `local-ready`
  - the preferred use of focused fix commits after a branch is published
- Align the narrative workflow document, `AGENTS.md`, and the repo-local closeout skill to the same three-part rule and terminology.
- Correct the reconciliation entry for `delivery-workflow-commit-guidance-followup-v1` so it is recorded as a superseded/reverted governance attempt rather than as the accepted final workflow wording.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `delivery-workflow`: add explicit commit-boundary governance for complex local development, first-push squashing, and post-push review fixes.

## Impact

- Formal spec: `openspec/specs/delivery-workflow/spec.md`
- Narrative/onboarding surfaces:
  - `obc-dev-spec/08_delivery_workflow.md`
  - `AGENTS.md`
  - `.codex/skills/change-closeout/SKILL.md`
- Audit trail:
  - `openspec/reconciliation/baseline-reconciliation-matrix.md`
  - `openspec/reconciliation/baseline-reconciliation-matrix.json`
