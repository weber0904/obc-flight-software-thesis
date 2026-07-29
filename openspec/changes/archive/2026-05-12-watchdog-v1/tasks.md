## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `watchdog-v1`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate watchdog-v1`.

## 2. Core Owner And Public Contract Migration

- [x] 2.1 Add the new `WatchdogSupervisor` component and migrate the existing CPU/RSS resource-monitoring behavior into it.
- [x] 2.2 Remove the separate `HealthMonitor` owner from the active baseline and move the public resource-monitoring surface to `watchdogSupervisor`.
- [x] 2.3 Update the core contract wiring, authority catalog/policy, and topology instances so the new owner path is consistent in both default and legacy topologies.
- [x] 2.4 Add the watchdog public surface for `GET_WATCHDOG_STATUS` and `SET_WATCHDOG_CONFIG`, plus the new internal watchdog fault mode-apply source.

## 3. Watchdog Runtime Behavior

- [x] 3.1 Define the shared watchdog source, recovery-level, config, and runtime-status vocabulary used by the new owner and hosted runtime.
- [x] 3.2 Add explicit heartbeat output wiring for `EpsBridge`, `EpsFdirController`, `ModeSafetyController`, and `CommController`.
- [x] 3.3 Implement tick-based freshness evaluation, warning/latch/suppress thresholds, truthful latched-fault vs `SAFE_REQUESTED` reporting, at-most-one watchdog `SAFE` escalation per fault epoch, and first-beat recovery clear behavior in `WatchdogSupervisor`.
- [x] 3.4 Add the supervisor-side watchdog feed/suppress hook and keep the claim bounded to feed eligibility rather than true target reset.
- [x] 3.5 Update hosted runtime status/reporting so watchdog aggregate and per-source truth is reviewable during probes.

## 4. Tests And Probe

- [x] 4.1 Add classic F' L2 coverage for `WatchdogSupervisor`, including migrated resource-monitoring behavior.
- [x] 4.2 Add direct helper tests for watchdog policy/config validation and aggregate clear rules.
- [x] 4.3 Rerun and keep green the affected regressions for `ModeSafetyController`, `EpsFdirController`, and hosted runtime health/status behavior.
- [x] 4.4 Add `scripts/run_watchdog_v1_probe.sh` with bounded beat-suppression control to prove healthy, warning, latched, suppressed, and recovered hosted behavior on `TopCcsds`.
- [x] 4.5 Record the results under `docs/test-records/watchdog-v1/README.md` and add a new hosted watchdog verification-path registry entry.

## 5. Verification And Closeout

- [x] 5.1 Run a fresh local verification gate with `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-watchdog-v1`.
- [x] 5.2 Run focused affected `ctest` coverage and `bash scripts/run_watchdog_v1_probe.sh` after the fresh build.
- [x] 5.3 Run `openspec validate watchdog-v1` and `openspec validate --specs`.
- [x] 5.4 Update canonical active-baseline docs: `README.md`, `docs/architecture/current-development-architecture.md`, `docs/verification-path-registry.md`, `docs/roadmap/README.md`, and the relevant roadmap note.
- [ ] 5.5 Archive the change only after implementation, verification, and spec sync are complete.
