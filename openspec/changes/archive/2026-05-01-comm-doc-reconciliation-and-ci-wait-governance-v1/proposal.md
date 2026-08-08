## Why

The COMM roadmap now identifies narrative/reporting reconciliation as the next low-risk step before deeper physical downlink work, and the top-level documentation still needs to reflect the latest COMM evidence boundaries. The delivery workflow also needs to make explicit that agents should open ready-for-review PRs by default and stop after reporting PR/CI state instead of repeatedly polling hosted CI when the developer will report the result.

## What Changes

- Reconcile current narrative surfaces against the registered COMM evidence boundaries.
- Update `docs/planning/comm-roadmap.md` to record the accepted next-step sequence from the merged planning snapshot.
- Add a delivery-workflow rule that pushed review-ready branches open as non-draft PRs by default so reviewer automation can run.
- Add a delivery-workflow rule that PR push handoff may stop at the CI-wait state once the PR/check link and pending state are reported.
- Preserve the existing rule that formal completion still requires CI green before merge or release.
- Do not add or modify runtime code, topology, scripts, simulators, validation paths, or component behavior.

## Capabilities

### New Capabilities

### Modified Capabilities
- `delivery-workflow`: Clarify ready PR creation and the agent/developer handoff behavior while a pushed PR is waiting for hosted CI.

## Impact

- Documentation and governance only.
- Affected surfaces may include `README.md`, `docs/planning/comm-roadmap.md`, selected architecture-review/reporting indexes or snapshot warnings, `openspec/specs/delivery-workflow/spec.md`, and governance evidence.
- No new runtime evidence or verification-path registry entry is introduced.
