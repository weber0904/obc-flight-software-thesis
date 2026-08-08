## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `core-system-contracts`, `eps-subsystem`, `adcs-subsystem`, `comm-subsystem`, `boot-update`, `mission-autonomy`, `verification-evidence`, and `verification-path-registry`.
- [x] 1.2 Validate the change artifacts before runtime implementation with `openspec validate multi-subsystem-fdir-v1`.

## 2. Shared Recovery Contracts And Runtime Wiring

- [x] 2.1 Extend shared recovery enums, source-count guards, reboot-cause mapping, and action/status vocabulary for ADCS, COMM, and `WATCHDOG_ADCS_FDIR`.
- [x] 2.2 Refactor `RecoveryExecutor` into short-lock decision plus lock-free action execution while keeping EPS and watchdog behavior intact.
- [x] 2.3 Add scheduled-poll health state to `AdcsBridge`, implement `AdcsFdirController`, and wire ADCS detector requests, clears, and watchdog supervision into the shared recovery path.
- [x] 2.4 Add bounded COMM detector policy and executor-owned `performRecoveryLinkFailoverForRuntime()` behavior, and remove detector-side fault actuation from `CommController`.
- [x] 2.5 Extend `BootManager` and shared runtime wiring so ADCS/COMM reboot-intent outcomes persist truthful reset-cause and recovery metadata.
- [x] 2.6 Update `TopCcsds` fast-group order and keep legacy `Top` build-safe with the new shared recovery contracts.

## 3. Focused Tests

- [x] 3.1 Expand `RecoveryExecutor` component/helper coverage for ADCS and COMM incident mapping, lock-outside-action execution, relatch escalation, and reboot-only boot-truth writes.
- [x] 3.2 Expand `AdcsBridge` coverage for scheduled-poll health and add classic `AdcsFdirController` coverage plus direct `AdcsFdirPolicy` tests.
- [x] 3.3 Expand `CommController` coverage so detector faults do not switch primary links directly, failover results stay reviewable, and direct `CommFdirPolicy` tests cover thresholds and clear rules.
- [x] 3.4 Expand `WatchdogSupervisor` and `BootManager` coverage for `WATCHDOG_ADCS_FDIR`, ADCS/COMM reboot truth, and unchanged detector-local status surfaces.
- [x] 3.5 Rerun the closest EPS, watchdog, recovery-executor, ADCS, COMM, and boot regressions after the new tests land.

## 4. Hosted Probe And Evidence

- [x] 4.1 Add `scripts/run_multi_subsystem_fdir_v1_probe.sh` with isolated runtime roots, ports, and same-root relaunch support.
- [x] 4.2 Prove EPS repeated timeout still reaches the shared executor and preserves the existing bounded reset, `SAFE`, clear, and relatch behavior.
- [x] 4.3 Prove ADCS repeated scheduled poll failure or no-valid-refresh reaches the shared recovery path and performs the bounded ADCS action and escalation flow.
- [x] 4.4 Prove COMM sustained primary-link failure reaches the shared recovery path, performs executor-owned failover or `SAFE`, and leaves structured recovery status truth.
- [x] 4.5 Record the proof in `evidence/records/multi-subsystem-fdir-v1/README.md` and update the verification-path registry for the new hosted path.

## 5. Verification And Closeout

- [x] 5.1 Run a fresh local verification build for the change.
- [x] 5.2 Run focused `ctest` commands, the earlier recovery regression probe, the new multi-subsystem probe, and the closest ADCS/COMM/EPS/watchdog/boot regressions after the fresh build.
- [x] 5.3 Run `openspec validate multi-subsystem-fdir-v1` and `openspec validate --specs`.
- [x] 5.4 Update canonical architecture, roadmap, README, and verification documents so the active baseline truth matches the new bounded multi-subsystem recovery ownership.
