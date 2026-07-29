## Why

The repository has a stable hosted OBC baseline, Raspberry Pi target integration, and governed comm paths, but it still lacks the scenario layer needed for the next extension phase. Before adding autonomy behaviors such as low-battery response, sun-tracking, detumbling, or ground-pass-aware flows, the project needs a repository-owned way to replay mission-context inputs into the hosted simulators without coupling the OBC directly to STK.

## What Changes

- Add a new repository-owned offline scenario replay capability that defines a simple scenario timeline format and a host-side scenario bridge for the existing simulators.
- Extend the hosted simulator layer so EPS can accept scenario-driven sunlight and battery state-of-charge inputs, while ADCS can accept scenario-seeded initial angular rates for deployment-style cases.
- Preserve the existing direct TCP to `fprime-gds` baseline and the current simulator transport split while making scenario context available for later autonomy and mission-validation work.
- Add first-version tests and evidence hooks for scenario replay behavior, and keep MissionExecutive, housekeeping/downlink, RF effects, and direct STK-to-OBC integration out of scope for this slice.

## Capabilities

### New Capabilities

- `scenario-driven-validation`: repository-owned offline scenario timeline and bridge that can replay scenario inputs into hosted simulators.

### Modified Capabilities

- `eps-subsystem`: extend the hosted EPS simulator contract to accept scenario-driven sunlight and battery state-of-charge inputs.
- `adcs-subsystem`: extend the hosted ADCS simulator contract to accept scenario-seeded deployment-rate initial conditions without replacing control-responsive simulator dynamics.
- `verification-evidence`: require reviewable evidence for the scenario replay architecture and its first governed simulator-level validation.

## Impact

- Affected code: `simulators/`, simulator integration tests, and simulator-facing docs.
- Affected systems: hosted EPS/ADCS simulator behavior, future scenario-driven SIL flows, and future autonomy validation work.
- No breaking changes to the current OBC runtime baseline, direct TCP/GDS path, Raspberry Pi deployment flow, or transparent link work.
