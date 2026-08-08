## Why

The checked-in verification matrix and inventory still describe the repo in a way that blurs true components together with helper and support modules. That makes it too easy to miss the real drift: later helper decomposition was reasonable, but classic component UT on later true components was inconsistent.

## What Changes

- Rewrite the verification matrix and its generator so they report component coverage and helper/support coverage separately.
- Make the inventory explicit about which modules are real F' components and whether classic F' L2 coverage exists for each one.
- Remove the misleading early-versus-late wording and replace it with the correct classification.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `verification-evidence`: require the formal matrix and inventory report to distinguish component coverage from helper/support coverage and to report remaining gaps against that rule.

## Impact

- Affected code: `scripts/report_verification_inventory.py`, `docs/verification-matrix.md`, and supporting evidence/docs.
- Affected systems: review accuracy, maintenance planning, and future auditability of test coverage.
- No change to flight runtime behavior or target integration behavior.
