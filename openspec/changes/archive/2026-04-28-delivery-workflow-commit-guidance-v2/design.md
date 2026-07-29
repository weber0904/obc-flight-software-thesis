## Context

The repository's real workflow has three distinct phases:

1. local development can keep rollback checkpoints for complex changes
2. the first push still requires one reviewable Conventional Commit at `local-ready`
3. once a branch is published, review follow-ups usually append small fix commits rather than rewriting published history

The earlier follow-up attempted to codify this, but it was reviewed, revised, partially merged, and then reverted. The current baseline is again the older workflow wording, while the audit trail still contains the superseded archived follow-up package.

## Goals / Non-Goals

**Goals:**
- Reintroduce the commit-boundary rule from a clean post-revert baseline.
- State the rule with unambiguous phase boundaries.
- Standardize terminology on `focused fix commits` and `published history`.
- Correct the reconciliation entry for the superseded follow-up.

**Non-Goals:**
- Changing CI, branch protection, or release rules.
- Rewriting or deleting the archived superseded follow-up package.
- Changing the repository's requirement that the first push / `local-ready` boundary uses one reviewable Conventional Commit.

## Decisions

- Use a dedicated `Complex Change Commit Boundaries` requirement in the formal spec.
  - Rationale: this rule is adjacent to `local-ready`, but clearer as its own requirement than as a paragraph appended to existing checkpoint text.
- Use `MAY` for local rollback commits.
  - Rationale: keeping rollback commits is allowed and encouraged for complex work, but not mandatory for every change.
- Use `SHOULD` for post-push focused fix commits.
  - Rationale: the repository strongly prefers this behavior after publication, but it remains a preference rather than an absolute prohibition on exceptional history repair.
- Keep `AGENTS.md` concise and repo-entrypoint-focused.
  - Rationale: it should mirror the canonical rule without turning into a second workflow spec.
- Mark the prior archived follow-up as superseded/reverted in the reconciliation layer.
  - Rationale: preserve audit history while making clear that its wording was not the accepted final state.

## Risks / Trade-offs

- [Risk] Another round of reviewer wording churn.
  - Mitigation: keep the rule phase-based, explicit, and terminologically consistent across all surfaces.
- [Risk] Audit trail confusion because the superseded archived follow-up remains in the repository.
  - Mitigation: update the matrix entry to state clearly that it was reverted/superseded before acceptance.

## Migration Plan

1. Add the new `delivery-workflow` delta spec and supporting narrative/onboarding updates.
2. Update the reconciliation entry for `delivery-workflow-commit-guidance-followup-v1`.
3. Validate specs and repository governance checks.
4. Archive this new change and add its own reconciliation entry.

## Open Questions

- None for this slice.
