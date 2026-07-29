## 1. OpenSpec Artifacts

- [x] 1.1 Create proposal, design, and delivery-workflow delta spec for the combined docs/governance change.

## 2. COMM Documentation Reconciliation

- [x] 2.1 Update current-facing README COMM baseline wording to match registered evidence boundaries.
- [x] 2.2 Update planning roadmap with the accepted next-step sequence and current reconciliation boundary.
- [x] 2.3 Add direct snapshot/staleness warnings to older architecture-review and reporting package roots that may be opened without the index.

## 3. CI Wait Governance

- [x] 3.1 Update `AGENTS.md`, `README.md`, narrative delivery workflow docs, and closeout skill with the CI-wait handoff rule.
- [x] 3.2 Add the default ready-for-review / non-draft PR rule so reviewer automation can trigger.

## 4. Evidence And Validation

- [x] 4.1 Add governance evidence naming sources checked, surfaces updated, and non-claims.
- [x] 4.2 Run `openspec validate comm-doc-reconciliation-and-ci-wait-governance-v1`.
- [x] 4.3 Run `openspec validate --specs`.
- [x] 4.4 Run `python3 scripts/check_repo_consistency.py`.
- [x] 4.5 Run `git diff --check`.
