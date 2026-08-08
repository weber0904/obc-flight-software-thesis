## Why

The active baseline already has a shared `RecoveryExecutor`, but it is still effectively a watchdog plus EPS closure path. ADCS and COMM now expose mature enough detector surfaces that the next formal baseline change should converge `EPS`, `ADCS`, and `COMM` onto one bounded shared recovery owner without over-claiming generic all-subsystem FDIR.

## What Changes

- Extend the shared recovery model so `RecoveryExecutor` becomes the single runtime owner for bounded `EPS`, `ADCS`, and `COMM` recovery progression and action execution.
- Keep `EpsFdirController` as the EPS detector, add a new scheduled-poll-only `AdcsFdirController`, and keep COMM detection inside `CommController` with a bounded policy helper.
- Refactor `RecoveryExecutor` so it decides incident progression under a short lock and executes mode, boot, EPS, and COMM recovery actions after the lock is released.
- Remove detector-side COMM failover actuation and route fault-driven failover, session revoke, and downlink-owner clear through executor-owned COMM recovery hooks.
- Extend boot/recovery metadata truth only for recovery-driven reboot intent, not for non-reboot recovery actions.
- Add focused component/helper coverage, a hosted `multi-subsystem-fdir-v1` probe, verification-path updates, and architecture/roadmap truth updates for the new baseline.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: broaden the bounded shared recovery status vocabulary to cover ADCS and COMM incidents plus executor-owned COMM recovery action truth.
- `eps-subsystem`: keep EPS timeout detection intact while making shared recovery ownership the formal active-baseline contract.
- `adcs-subsystem`: add scheduled-poll health, first-version ADCS FDIR detection boundaries, and explicit separation between local sensor faults and shared recovery incidents.
- `comm-subsystem`: add first-version COMM FDIR detector and executor-owned recovery behavior while keeping operator link-role policy separate from fault policy.
- `boot-update`: broaden persisted recovery reboot truth to cover ADCS and COMM reboot-intent outcomes without treating non-reboot actions as boot reset causes.
- `mission-autonomy`: expand the active shared recovery owner from watchdog plus EPS to a bounded `EPS + ADCS + COMM` recovery path with deterministic relatch and escalation rules.
- `verification-evidence`: require reviewable unit, component, and hosted proof for the new three-subsystem shared recovery closure.
- `verification-path-registry`: register the hosted multi-subsystem FDIR path separately from the earlier watchdog-only and EPS-only paths.

## Impact

- Affected runtime/code areas: `RecoveryExecutor`, `AdcsBridge`, new `AdcsFdirController`, `CommController`, `WatchdogSupervisor`, `BootManager`, `TopCcsds`, legacy `Top`, shared recovery types, and hosted runtime/probe surfaces.
- Affected public/runtime surfaces: `GET_RECOVERY_STATUS`, `GET_RESET_CAUSE`, recovery incident/action telemetry and events, ADCS/COMM detector status truth, COMM recovery failover result truth, and persisted reboot metadata.
- Affected verification/docs: new hosted probe and test record, updated verification-path registry, roadmap, README, current-development architecture, and OpenSpec delta specs.
- Out of scope: GPS, payload, TTC/pass scheduling, storage-health recovery, persistent event-log infrastructure, COMM RF recovery, target-hardware reset proof, or a generic all-subsystem FDIR platform.
