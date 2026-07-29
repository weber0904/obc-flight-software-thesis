## Why

The project now has multiple implemented capability slices and a growing set of reviewable evidence records, but the verification gate is still driven manually from ad hoc terminal commands. The final queued change needs to make that gate repeatable in-repo by providing a single verification script, a repository-local CI workflow, and a reusable evidence template so later capability work can follow one consistent path.

## What Changes

- Add a repository-local verification gate script under `scripts/` that runs the normal build, UT build, full test gate, and `openspec validate --specs`.
- Add a GitHub Actions workflow under `.github/workflows/` that provisions dependencies, checks out the submodule, and runs the shared verification script.
- Add a reusable evidence template under `docs/test-records/templates/` and record the initial verification-CI slice evidence.
- Update the narrative workflow and verification-evidence source documents to reference the shared script and template.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `delivery-workflow`: add the repository-local CI workflow and shared verification entrypoint.
- `verification-evidence`: add the reusable evidence template and the first CI-gate evidence record.

## Impact

- Affected code: `.github/workflows/`, `scripts/`
- Affected docs: `docs/test-records/`, `obc-dev-spec/07_verification_evidence.md`, `obc-dev-spec/08_delivery_workflow.md`
- Dependencies: GitHub Actions Ubuntu runner packages, Python virtual environment setup, Node-based `openspec` CLI in CI
