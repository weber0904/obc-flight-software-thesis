## Context

The repository currently mixes four governance layers:

1. the formal OpenSpec main spec
2. the narrative workflow document
3. the repo-root agent entrypoint
4. CI-side helper scripts and generated review surfaces

The strongest parts of that design are:

- one shared CI gate entrypoint
- explicit validation-path governance
- a real component-test baseline checker
- a reconciliation layer that lets reviewers trace archived changes to current capabilities and evidence

The weak parts are:

- a formal spec that still carries historical queue and governance-only assumptions
- CI checks that block PRs on fragile markdown maintenance
- generic OpenSpec-generated Codex skills were being treated as repo-specific governance files, which invites drift and accidental hand-edits
- a checked-in reconciliation Markdown file that duplicates the JSON source by hand
- a branch-history rule that is stricter than actual practice and duplicates what squash-merge can already guarantee on `main`

## Goals / Non-Goals

**Goals:**

- Preserve the repo-specific governance that protects real engineering risks.
- Remove or demote fragile checks that validate formatting or onboarding prose rather than product or traceability risk.
- Make the formal spec, narrative guide, agent entrypoint, CI gate, and closeout skill say the same thing.
- Keep `baseline-gate` as the single required CI job while allowing a narrow docs-only fast path for architecture-review documents.

**Non-Goals:**

- Rewriting existing archive history or removing archived governance changes.
- Replacing OpenSpec with a different process.
- Automating releases or changelog generation.
- Introducing broad path-based workflow skips that leave required checks pending.

## Decisions

- Keep `openspec/specs/delivery-workflow/spec.md` as the only normative workflow source.
  - Rationale: adding `CONTRIBUTING.md` or another rule document would create a fourth active rule surface.
- Make `AGENTS.md` an index with only three fixed read-first documents.
  - Rationale: agents need a stable entrypoint, but not a second narrative workflow spec.
- Keep `baseline-gate` as one required CI job and classify PRs inside the workflow.
  - Rationale: GitHub leaves required checks pending when an entire workflow is skipped by path filters.
- Limit docs-fast-path to `docs/architecture-review/**`.
  - Rationale: that path contains report-only content that should not rebuild the project, while files like `README.md`, `AGENTS.md`, `openspec/**`, or evidence docs still affect formal governance or traceability.
- Remove `check_agent_entrypoint.py` entirely.
  - Rationale: agent onboarding links are too low-value and too brittle to justify blocking PRs.
- Keep `check_repo_consistency.py`, but rewrite it to validate only formal spec quality, matrix integrity, archive coverage, evidence-path existence, and generated Markdown consistency.
  - Rationale: those checks directly support professional auditability.
- Keep `verification-matrix.md` as a manually curated review surface with a disclaimer instead of trying to auto-generate its richer narrative content.
  - Rationale: the current file contains capability-level interpretation that is not derivable from the inventory library alone.
- Normalize mainline history around squash-merge and PR-title Conventional Commits.
  - Rationale: this gives a clean `main` history without forcing every branch to be rebased into one commit before first push.

## Risks / Trade-offs

- [Risk] Rewriting the formal workflow spec and the narrative guide in one PR can introduce wording drift.
  - Mitigation: update `delivery-workflow/spec.md`, `08_delivery_workflow.md`, `AGENTS.md`, and the closeout skill together, then run the rewritten consistency checks and full baseline gate in the same branch.
- [Risk] Docs-fast-path may accidentally classify a governance-relevant docs change as "docs only."
  - Mitigation: keep the allowlist narrow and limited to `docs/architecture-review/**`.
- [Risk] Removing the verification-matrix sync gate may let the review surface go stale.
  - Mitigation: add a disclaimer to the document and keep the code-side coverage checks authoritative in CI.
- [Risk] Generated reconciliation Markdown can still drift if developers forget to regenerate it.
  - Mitigation: make `check_repo_consistency.py` compare the generated output to the checked-in Markdown and fail with an explicit regeneration hint.

## Migration Plan

1. Create this change and add the governance artifacts first.
2. Rewrite the formal workflow spec and all checked-in companion documents in the same branch.
3. Rewrite the CI helper scripts and add the reconciliation Markdown generator.
4. Keep OpenSpec-generated Codex skills under OpenSpec tool management, stop treating them as repo-specific governance files, and remove the `AGENTS.md` checker.
5. Run the full baseline gate plus direct consistency checks.
6. Mark tasks complete and archive the change locally; do not push until developer approval.

## Open Questions

- None for implementation. GitHub branch-protection settings that live outside the repo remain a maintainer follow-up after this branch lands.
