## Why

The repository's delivery workflow, agent onboarding, and CI governance currently enforce a mix of high-value product-risk checks and low-value documentation bookkeeping. Earlier governance changes accumulated overlapping rules across the formal spec, the narrative workflow document, `AGENTS.md`, repo-local skills, and CI helper scripts. The result is a baseline that is reviewable but heavier than necessary, with several rules that are brittle, duplicated, or no longer aligned with actual practice.

This change resets the governance model around a smaller set of enforceable rules:

- the formal workflow spec remains the only normative source
- `AGENTS.md` becomes a concise entrypoint instead of a second workflow description
- CI keeps product-risk and core traceability checks, but drops fragile documentation gates
- reconciliation data gets one machine-maintained source with a generated human-readable view
- PR/mainline history is normalized around squash-merge, while branch-local rebase cleanup remains optional

## What Changes

- Rewrite the formal `delivery-workflow` requirement set to:
  - remove the false "only one governance-only change" rule
  - remove historical baseline-queue governance from the active workflow contract
  - define OpenSpec applicability, branch naming, PR title, squash-merge, and local-ready boundaries more precisely
  - drop the repo-local `AGENTS.md` entrypoint checker from the formal required gates
- Align the narrative workflow document, `AGENTS.md`, `README.md`, and the closeout skill to the same governance model without turning any of them into competing rulebooks.
- Replace reconciliation maintenance drift with one checked-in JSON source and a generated Markdown review surface.
- Simplify the shared verification gate so PR CI enforces build/test/spec integrity and core traceability, while a narrow docs-only fast path avoids full builds for audit/report-only PRs under `docs/architecture-review/**`.
- Stop treating generic OpenSpec-generated Codex skills as repo-specific governance files; keep them under OpenSpec tool management and refresh them through `openspec update` when needed.

## Capabilities

### New Capabilities
- None.

### Modified Capabilities
- `delivery-workflow`: narrow the formal governance contract to enforceable workflow, CI, branch, and traceability rules.

## Impact

- Formal governance surfaces:
  - `openspec/specs/delivery-workflow/spec.md`
  - `obc-dev-spec/08_delivery_workflow.md`
  - `AGENTS.md`
  - `README.md`
- CI / traceability tooling:
  - `.github/workflows/verification-ci.yml`
  - `scripts/run_verification_ci.sh`
  - `scripts/check_repo_consistency.py`
  - `scripts/check_component_test_baseline.py`
  - `scripts/generate_reconciliation_matrix_md.py`
- Review surfaces:
  - `docs/baseline-reconciliation-matrix.json`
  - `docs/baseline-reconciliation-matrix.md`
  - `docs/verification-matrix.md`
- Repo-local and tool-managed skills:
  - `.codex/skills/change-closeout/SKILL.md`
  - retention of OpenSpec-managed `.codex/skills/openspec-*`
