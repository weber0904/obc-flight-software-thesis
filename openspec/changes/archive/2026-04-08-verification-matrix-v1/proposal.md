## Why

The repository now has a substantial mix of classic component UTs, custom integration tests, hosted probes, Raspberry Pi evidence, and constrained validation records, but there is still no single formal matrix that explains capability-by-capability what is actually covered today. That gap makes it too easy to overestimate coverage, especially in newer slices where tests shifted from classic F' harnesses toward integration and probe-driven validation.

## What Changes

- Add a formal verification matrix that lists each current capability's L1, L2, L3, and L4 coverage together with constrained gaps and notable weak spots.
- Add a repo-local verification inventory tool that aggregates registered tests, baseline gate entrypoints, hosted probes, and evidence directories from the current repository state.
- Update the verification-evidence capability so the matrix and inventory report become governed parts of the maintenance workflow instead of ad hoc review notes.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `verification-evidence`: require a formal capability-level verification matrix and a repo-owned inventory/report tool that surfaces the registered test/probe/evidence landscape.

## Impact

- Affected code: new repo-local verification inventory tool.
- Affected systems: reviewability of current coverage, maintenance planning, and follow-on UT backfill prioritization.
- No change to flight runtime behavior, simulator behavior, target integration behavior, or command interfaces.
