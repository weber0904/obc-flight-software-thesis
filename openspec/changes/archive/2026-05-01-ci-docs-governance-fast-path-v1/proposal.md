## Why

The repository's required `baseline-gate` currently has only a narrow fast path for `docs/architecture-review/**`. Other documentation-only and governance-only pull requests still run the full F' generate/build/UT gate even when no runtime, topology, script behavior, or build configuration can be affected. That adds avoidable CI time while still leaving the required check model intact.

## What Changes

- Add a repo-local change-scope classifier that returns `lightweight` only for an explicit whitelist of documentation and governance paths; all unknown paths default to `full`.
- Replace `docs-fast-path` with `lightweight` mode in the shared verification script.
- Keep the GitHub Actions workflow always running as the single required `baseline-gate`, while using the classifier inside the job to decide whether to skip F' build/test steps.
- Keep lightweight mode guarded by static governance checks: repo consistency, component-test baseline inventory, retired-path checks, and `openspec validate --specs`.
- Update delivery-workflow specs and narrative docs to describe the new verification scope classification.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `delivery-workflow`: adds conservative PR verification-scope classification while preserving the single required baseline gate.
- `verification-evidence`: records evidence for the classifier, lightweight gate, full gate, and OpenSpec validation.

## Impact

- Affected CI surfaces: `.github/workflows/verification-ci.yml`, `scripts/run_verification_ci.sh`, and `scripts/classify_change_scope.sh`.
- Affected governance docs: `openspec/specs/delivery-workflow/spec.md`, `obc-dev-spec/08_delivery_workflow.md`, `README.md`, and reconciliation/evidence records.
- This change itself is expected to run full CI because it modifies workflow and script files.
