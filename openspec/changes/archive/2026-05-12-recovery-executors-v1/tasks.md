## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `recovery-executors-v1`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate recovery-executors-v1`.

## 2. Shared Recovery Contracts And Runtime Wiring

- [x] 2.1 Add shared recovery types/interfaces plus the new `RecoveryExecutor` component with incident, level, and action-status surfaces.
- [x] 2.2 Rewire `WatchdogSupervisor` to emit shared recovery requests/clears while preserving watchdog-local freshness, suppression, and `GET_WATCHDOG_STATUS` truth.
- [x] 2.3 Rewire `EpsFdirController` to emit shared recovery requests/clears while preserving EPS-local retry, latch, and first-success clear behavior.
- [x] 2.4 Extend `BootManager` and `BootMetadataStore` with reset-cause, boot-count, consecutive-reset, and boot-safe-fallback truth plus `GET_RESET_CAUSE` and `GET_BOOT_COUNT`.
- [x] 2.5 Add hosted runtime reboot-hook support and wire `RecoveryExecutor`, `BootManager`, `EpsBridge`, and `ModeManager` into both `TopCcsds` and legacy `Top`.
- [x] 2.6 Move onboard-state and housekeeping reboot-count truth to boot metadata and update authority/catalog policy for the new status commands.

## 3. Focused Tests

- [x] 3.1 Add `RecoveryExecutor` classic component coverage for incident open/clear, single-shot action gating, EPS reset execution, watchdog reboot escalation, and status command output.
- [x] 3.2 Expand `WatchdogSupervisor` component/helper coverage for shared recovery request emission, no direct mode-ownership regression, and unchanged local watchdog truth.
- [x] 3.3 Expand `EpsFdirController` component/helper/integration coverage for shared recovery request emission, first-success clear, relatch escalation, and EPS reset handoff.
- [x] 3.4 Expand `BootManager` component/helper coverage for metadata schema upgrade, boot counter truth, reset cause reporting, consecutive-reset safe clamp, and stable-ack clearing.
- [x] 3.5 Rerun the closest affected regressions for watchdog, EPS FDIR, boot trust, topology/runtime snapshots, and authority policy surfaces.

## 4. Hosted Probe And Evidence

- [x] 4.1 Add `scripts/run_recovery_executors_v1_probe.sh` with isolated runtime roots and ports plus same-root relaunch for reboot-equivalent proof.
- [x] 4.2 Cover a watchdog stale case that records restart intent, requests `SAFE`, and escalates to reboot when suppression persists.
- [x] 4.3 Cover an EPS timeout case that enters the same executor path, performs EPS reset, requests `SAFE`, and escalates on repeated failure.
- [x] 4.4 Cover reboot-relaunch truth for `reset_cause`, `boot_count`, `consecutive_reset_count`, boot-safe fallback, and `GET_RECOVERY_STATUS`.
- [x] 4.5 Record evidence under `evidence/records/recovery-executors-v1/README.md` and update the verification-path registry entry set for the new hosted path.

## 5. Verification And Closeout

- [x] 5.1 Run a fresh local verification build/gate for the change.
- [x] 5.2 Run focused `ctest` commands, the new hosted recovery probe, and the relevant watchdog/EPS/boot regressions after the fresh build.
- [x] 5.3 Run `openspec validate recovery-executors-v1` and `openspec validate --specs`.
- [x] 5.4 Archive/sync the change and update canonical roadmap/architecture/verification documents for the new active baseline truth.
