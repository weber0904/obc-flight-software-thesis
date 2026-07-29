## 1. Define the CI and evidence standardization slice

- [x] 1.1 Add the change proposal, design notes, and spec deltas for `verification-ci-v1`
- [x] 1.2 Reconcile the narrative verification and delivery source documents with the new shared script and template

## 2. Add shared verification automation

- [x] 2.1 Add the repo-local verification gate script under `scripts/`
- [x] 2.2 Add the repository-local GitHub Actions workflow under `.github/workflows/`

## 3. Add evidence templates and records

- [x] 3.1 Add a reusable evidence template under `docs/test-records/templates/`
- [x] 3.2 Record the implementation and local verification results under `docs/test-records/verification-ci-v1/`

## 4. Validate and close the change

- [x] 4.1 Run the shared verification script locally and confirm the baseline gate passes
- [x] 4.2 Run `openspec validate verification-ci-v1` and archive the change after the implementation and evidence are complete
