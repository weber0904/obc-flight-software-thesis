## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md` for the clarification-only target dual-link claim
  slice and lock the modified capabilities.
- [x] 1.2 Add delta specs for `comm-subsystem`, `ground-ttc-gateway`,
  `interface-contract-index`, `verification-evidence`, and
  `verification-path-registry`.
- [x] 1.3 Write `design.md` with the exact future claim, oracle precedence,
  phase model, PASS/FAIL table, non-claims, and later-proof recommendation.

## 2. Main Specs And Current Docs

- [x] 2.1 Update the corresponding main specs under `openspec/specs/` so the
  frozen future boundary exists outside the change delta too.
- [x] 2.2 Update current architecture, roadmap, interfaces, runbooks, and
  registry citation notes so they consistently distinguish current proof from
  the newly frozen future target-bearing boundary.
- [x] 2.3 Keep all touched wording explicit that this change proves no new
  simultaneous target path.

## 3. Validation And Closeout Wording

- [x] 3.1 Run `openspec validate target-dual-link-claim-oracle-clarification-v1`.
- [x] 3.2 Run `openspec validate --specs`.
- [x] 3.3 Run `python3 scripts/check_documentation_governance.py` and
  `python3 scripts/check_repo_consistency.py`.
- [x] 3.4 Mark the task list complete and ensure the final summary states the
  exact future claim, oracle, PASS boundary, non-claims, remaining later-proof
  work, and why this clarification slice stayed separate from implementation.
