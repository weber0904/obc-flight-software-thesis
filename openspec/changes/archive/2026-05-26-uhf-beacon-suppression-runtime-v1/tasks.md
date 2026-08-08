## 1. OpenSpec Artifacts

- [x] 1.1 Create `proposal.md`, `design.md`, and `tasks.md` for `uhf-beacon-suppression-runtime-v1`.

## 2. Runtime Implementation

- [x] 2.1 Add a bounded accepted-session observer surface from `CommandIngressAuthority` to `CommController`.
- [x] 2.2 Implement the `CommController` suppress state machine with fixed `60`-tick inactivity timeout and immediate clear on session invalidation.
- [x] 2.3 Add a bounded runtime suppress gate to `BeaconPublisher` and wire it from `CommController`.
- [x] 2.4 Extend runtime status, telemetry, and events so suppress truth is reviewable.
- [x] 2.5 Extend target UHF launch/service surfaces only as needed for probe-owned beacon capture, keeping the feature optional and off by default.

## 3. Focused Verification

- [x] 3.1 Add `CommController` unit coverage for start, refresh, timeout resume, replace, revoke, failover clear, and negative non-trigger behavior.
- [x] 3.2 Add `CommandIngressAuthority` coverage that observer notifications fire only for accepted UHF lifecycle/activity events.
- [x] 3.3 Add `BeaconPublisher` coverage for suppressed silent ticks and resumed emission.

## 4. Governed Probes And Evidence

- [x] 4.1 Add a hosted node-6 beacon suppression probe that proves start, hold, resume, and at least one negative case.
- [x] 4.2 Add a target CAN quiet-UHF node-6 beacon suppression probe with probe-owned beacon capture and journal markers.
- [x] 4.3 Add a change-specific evidence record that states path under test, newly proven behavior, and remaining non-claims.
- [x] 4.4 Update the verification-path registry for the newly proven suppress/runtime behavior only.

## 5. Specs And Docs

- [x] 5.1 Update `comm-subsystem`, `live-beacon-broadcast`, `interface-contract-index`, `verification-evidence`, and `verification-path-registry` delta specs.
- [x] 5.2 Sync current architecture, interfaces, roadmap/baseline, verification registry, and COMM matrix/runbook wording with the new runtime truth.

## 6. Validation

- [x] 6.1 Run fresh local verification for touched code and probes.
- [x] 6.2 Run `openspec validate uhf-beacon-suppression-runtime-v1`.
- [x] 6.3 Run `openspec validate --specs`.
