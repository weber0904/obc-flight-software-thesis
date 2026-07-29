## Context

The repository already enforces a `local-ready` checkpoint that requires one reviewable Conventional Commit before the first push. In practice, complex hardware and integration slices also benefit from meaningful intermediate local commits as rollback points. The prior wording update captured that intent, but it did not update the formal `delivery-workflow` spec and left ambiguity about whether published branches should be force-rewritten after review.

## Goals / Non-Goals

**Goals:**
- Make the rollback-commit rule explicit in the formal `delivery-workflow` spec.
- State clearly that the single-commit rule applies before the first push / `local-ready`.
- State clearly that published branches default to focused fix commits for review follow-ups.
- Keep formal spec, narrative workflow, onboarding, and closeout skill terminology aligned.

**Non-Goals:**
- Changing the repository's CI gating behavior.
- Introducing a new branching model.
- Requiring history rewrites after a branch has been published.

## Decisions

- Add a new `delivery-workflow` requirement instead of overloading the existing `Change Completion Checkpoints` requirement.
  - Rationale: the commit-boundary rule is related to `local-ready`, but it governs development history shape and post-push review behavior more directly than the completion checkpoint itself.
- Use `SHOULD` for published-branch follow-up commits.
  - Rationale: this is a strong workflow preference and the repository's default expectation, but it still leaves room for exceptional governed cases where a rewrite may be justified.
- Standardize on `focused fix commits`.
  - Rationale: one term should appear across formal spec, narrative workflow, and agent-facing guidance.

## Risks / Trade-offs

- [Risk] This follow-up is documentation/spec only and may appear heavier than the code change it describes.
  - Mitigation: keep the scope limited to one capability and three synced documentation surfaces.
- [Risk] Overstating the rule could imply force-rewrites are forbidden in every circumstance.
  - Mitigation: use `SHOULD` instead of `SHALL` for published-branch follow-up commits.

## Migration Plan

1. Add the delta spec for `delivery-workflow`.
2. Align `AGENTS.md`, `08_delivery_workflow.md`, and the closeout skill with the formal wording.
3. Validate specs and repository checks.
4. Archive the change so the corrected workflow rule is visible in governed history.

## Open Questions

- None for this follow-up.
