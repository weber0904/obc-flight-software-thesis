## Why

The current repository has already frozen the UHF beacon suppress/resume policy boundary, but that rule is still only wording. This change is needed now to turn that policy into reviewable runtime truth on the active baseline, with explicit state, transitions, timeout semantics, and governed evidence.

## What Changes

- Implement session-aware UHF beacon suppress/resume runtime behavior on the active `TopCcsds` baseline.
- Keep `CommController` as the sole suppress/resume policy owner and keep `BeaconPublisher` limited to cadence, encode, and bounded emission control.
- Add bounded runtime observability for suppress state, owner session, inactivity timeout, transition reason, and final resume.
- Add governed hosted and target quiet-UHF proofs that show suppress start, maintenance, timeout resume, and negative non-trigger behavior.
- Update formal specs, current docs, and verification records so the capability is no longer described as a residual gap.

## Capabilities

### New Capabilities

- `uhf-beacon-suppression-runtime`: reviewable runtime closure for COMM-owned UHF beacon suppress/resume behavior on the active baseline

### Modified Capabilities

- `comm-subsystem`: add the runtime suppress/resume rule, owner boundary, timeout semantics, and observability contract
- `live-beacon-broadcast`: update the beacon capability from policy-only wording to implemented COMM-owned suppress/runtime behavior
- `interface-contract-index`: record the new runtime fields, owners, units, and state semantics
- `verification-evidence`: require reviewable hosted and target evidence for suppress start, hold, resume, and negative cases
- `verification-path-registry`: register the newly proven suppress/runtime behavior on the governed hosted and target node-6 paths

## Impact

- Affected code: `CommandIngressAuthority`, `CommController`, `BeaconPublisher`, `TopCcsds` runtime wiring, hosted runtime status output, and governed probe/service wrappers
- Affected verification: focused component/unit coverage, new hosted probe, new target quiet-UHF probe, evidence record, and verification-path registry updates
- Affected systems: active hosted node-6 CCSDS path and target CAN quiet-UHF node-6 path only
