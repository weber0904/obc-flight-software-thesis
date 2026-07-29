## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, delivery-workflow delta spec, and task list for `ci-docs-governance-fast-path-v1`.

## 2. CI Scope Classification

- [x] 2.1 Add `scripts/classify_change_scope.sh` with whitelist/default-full classification.
- [x] 2.2 Update `.github/workflows/verification-ci.yml` to use the classifier while keeping `baseline-gate` always running.
- [x] 2.3 Update `scripts/run_verification_ci.sh` to replace `docs-fast-path` with `lightweight` mode.

## 3. Governance Documentation

- [x] 3.1 Update delivery-workflow specs through the OpenSpec delta.
- [x] 3.2 Update README and narrative delivery workflow docs to describe lightweight verification.
- [x] 3.3 Add evidence and reconciliation-matrix coverage.

## 4. Validation

- [x] 4.1 Run classifier shell checks for expected lightweight and full cases.
- [x] 4.2 Run local lightweight verification mode.
- [x] 4.3 Run full local verification gate.
- [x] 4.4 Run `openspec validate ci-docs-governance-fast-path-v1`.
- [x] 4.5 Run `openspec validate --specs`.
- [x] 4.6 Run `python3 scripts/check_repo_consistency.py`.
- [x] 4.7 Run `git diff --check`.
