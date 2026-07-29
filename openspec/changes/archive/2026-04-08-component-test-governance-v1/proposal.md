## Why

The repository now has a clear split between real F' components and helper or support logic, but the formal workflow does not yet enforce the corresponding testing rule. As a result, later true components were allowed to ship without the same classic F' L2 harness discipline that earlier components followed.

## What Changes

- Update the formal delivery and verification specs so they explicitly distinguish F' components from helper or support modules.
- Require every real repository component derived from `*ComponentBase` to carry a classic F' L2 component harness unless a future governed exception is approved.
- Require helper or support logic with nontrivial behavior to keep plain L1 unit tests, while forbidding those tests from being treated as a substitute for missing component UT.
- Add a repo-local component-test-baseline checker and wire it into the shared verification gate.
- Update the repo entrypoints and local skill guidance so future changes inherit the rule up front.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `delivery-workflow`: add the minimum component-test baseline rule and require the repo-local checker in the standard gate.
- `verification-evidence`: distinguish component L2 coverage from helper L1 coverage and require both to remain reviewable as separate layers.

## Impact

- Affected code: workflow specs, agent/workflow entry docs, one new repo-local checker, and the shared baseline verification gate.
- Affected systems: delivery governance, verification reviewability, and future component onboarding discipline.
- No change to flight runtime, simulator runtime, or target runtime behavior.
