## Why

The active baseline already detects watchdog stale faults and repeated EPS poll failures, but both slices stop at detection, bounded escalation, and at-most-one `SAFE` request. There is still no shared runtime owner for recovery progression, no truthful reboot/reset intent closure, and no persistent boot metadata that can explain what happened after a recovery-triggered restart.

This change closes that next formal baseline gap with a single governed PR. It converges the existing watchdog and EPS-only FDIR slices onto one shared recovery executor path, adds bounded hosted proof for reboot-equivalent closure, and makes reset/boot truth reviewable without over-claiming all-subsystem FDIR, target hardware watchdog, or full flight secure boot.

## What Changes

- Add a new focused `RecoveryExecutor` component as the single owner for shared recovery incident normalization, recovery-level progression, and bounded recovery action execution.
- Keep `WatchdogSupervisor` as the bounded software-watchdog detector and `EpsFdirController` as the EPS-timeout detector, but change both to emit shared recovery requests/clears instead of owning `SAFE` escalation directly.
- Define the first bounded shared recovery action set:
  - process-restart intent as a reviewable `R2` action
  - EPS subsystem/interface reset execution as a real `R3` action
  - `SAFE` fallback through the normal internal mode path as a real `R5` action
  - hosted OBC reboot-equivalent execution as a real `R6` action
- Extend boot metadata truth with persisted `reset_cause`, `boot_count`, `consecutive_reset_count`, last recovery source/level, and a boot-after-recovery `SAFE` fallback clamp.
- Add bounded public status surfaces for `GET_RESET_CAUSE`, `GET_BOOT_COUNT`, and `GET_RECOVERY_STATUS` while intentionally not adding generic `FORCE_PROCESS_RESTART` or `FORCE_SUBSYSTEM_RESET`.
- Add focused component/helper tests, hosted recovery-executor probe evidence, verification-path updates, and architecture/roadmap truth updates for the new active baseline.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `mission-autonomy`: add the shared `RecoveryExecutor` owner and reroute watchdog/EPS detector escalation through the shared recovery path without broadening `ModeSafetyController`.
- `core-system-contracts`: extend the bounded operator-visible core status surface with `GET_RESET_CAUSE`, `GET_BOOT_COUNT`, and `GET_RECOVERY_STATUS` while keeping generic forced restart/reset commands out of v1.
- `boot-update`: extend the `BootManager` contract and file-backed metadata model to preserve reset-cause and recovery-boot truth across restart-equivalent flows.
- `resource-storage`: keep the new recovery boot metadata fields under the existing file-backed persistent-storage model and governed runtime roots.
- `verification-evidence`: require reviewable unit, integration, hosted-probe, and OpenSpec-validation evidence for the shared recovery executor closure.

## Impact

- Affected runtime/code areas: new `OBC/Components/RecoveryExecutor`, `WatchdogSupervisor`, `EpsFdirController`, `BootManager`, `BootMetadataStore`, `EpsBridge` recovery hook use, runtime service hooks, topology wiring, authority catalog/policy, and onboard snapshot providers.
- Affected public/runtime surfaces: `GET_RECOVERY_STATUS`, `GET_RESET_CAUSE`, `GET_BOOT_COUNT`, new recovery events/telemetry, updated boot metadata schema, and truthful reboot-count sourcing for HK/onboard state.
- Affected verification/docs: new hosted recovery probe, new `evidence/records/recovery-executors-v1/README.md`, updated verification-path registry, roadmap, architecture truth, and OpenSpec delta specs/archive flow.
- Out of scope: broad all-subsystem FDIR, TTC/pass-window or payload behavior, persistent event log, hardware watchdog proof, target hardware reboot proof, full secure boot redesign, and generic operator `FORCE_*` surfaces.
