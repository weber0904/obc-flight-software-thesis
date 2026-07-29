## 1. Formal Governance Reset

- [x] 1.1 Rewrite the `delivery-workflow` formal spec to remove historical/governance-only assumptions and define the reduced normative workflow rules.
- [x] 1.2 Align `obc-dev-spec/08_delivery_workflow.md`, `AGENTS.md`, `README.md`, and `.codex/skills/change-closeout/SKILL.md` to the same governance model.

## 2. CI And Traceability Tooling

- [x] 2.1 Rewrite `scripts/check_repo_consistency.py` to validate only spec-purpose quality, capability/matrix integrity, archived-change coverage, evidence-path existence, and generated Markdown consistency.
- [x] 2.2 Add `scripts/generate_reconciliation_matrix_md.py` and regenerate `docs/baseline-reconciliation-matrix.md` from the JSON source.
- [x] 2.3 Rewrite `scripts/check_component_test_baseline.py` so it enforces code-side coverage only and no longer blocks on `docs/verification-matrix.md`.
- [x] 2.4 Simplify `.github/workflows/verification-ci.yml` and `scripts/run_verification_ci.sh` to remove `push: main`, remove fragile governance checks, and add the narrow docs-fast-path flow.

## 3. Agent Surface Cleanup

- [x] 3.1 Delete `scripts/check_agent_entrypoint.py`.
- [x] 3.2 Restore the generic `.codex/skills/openspec-*` Codex skills and treat them as OpenSpec-managed generated artifacts instead of repo-specific governance files.
- [x] 3.3 Add a PR template that captures `OpenSpec`, verification, and risk/rollback summary fields.

## 4. Review Surfaces And Validation

- [x] 4.1 Add a non-authoritative disclaimer to `docs/verification-matrix.md`.
- [x] 4.2 Run `python3 scripts/check_repo_consistency.py`, `python3 scripts/check_component_test_baseline.py`, `openspec validate --specs`, and the full `scripts/run_verification_ci.sh` baseline gate.
- [x] 4.3 Mark this change ready for local archive after the rewritten governance surfaces and CI checks agree.
