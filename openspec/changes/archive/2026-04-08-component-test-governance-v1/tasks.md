## 1. Formal Governance Updates

- [x] 1.1 Update the `delivery-workflow` delta spec to require classic F' L2 harnesses for all real repository components and to require the repo-local baseline checker in the shared gate.
- [x] 1.2 Update the `verification-evidence` delta spec to distinguish component L2 coverage from helper L1 coverage and forbid helper tests from standing in for missing component UT.
- [x] 1.3 Add a checked-in evidence record for this governance change.

## 2. Repo-Local Enforcement

- [x] 2.1 Add `scripts/check_component_test_baseline.py` to enumerate real F' components and fail when a component lacks classic F' UT registration.
- [x] 2.2 Update `scripts/run_verification_ci.sh` so the shared baseline gate runs the new checker.
- [x] 2.3 Update `AGENTS.md` and `.codex/skills/change-closeout/SKILL.md` so future work is told this rule before implementation starts.

## 3. Validation

- [x] 3.1 Run `python3 scripts/check_component_test_baseline.py`.
- [x] 3.2 Run `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`.
- [x] 3.3 Run `openspec validate component-test-governance-v1` and `openspec validate --specs`.

## 4. Finalization

- [x] 4.1 Archive `component-test-governance-v1` after the workflow, checker, and evidence are aligned.
- [x] 4.2 Prepare the change for governed closeout on the dedicated feature branch.
