## Why

The active `TopCcsds` baseline now has bounded SoC mode safety, authenticated command/session policy, COMM link-role runtime policy, and EPS-only timeout FDIR, but it still has no runtime owner for process-heartbeat supervision. `HealthMonitor` remains only a narrow CPU/RSS threshold surface, so the repo still cannot answer which active runtime owners are supervised, how heartbeat freshness is judged, or what deterministic action follows a stale runtime owner.

This change closes that next vertical gap with a first-version software watchdog on the active baseline. It adds one explicit watchdog owner, bounded supervised sources, deterministic escalation and recovery rules, and a truthful supervisor-side hardware-watchdog feed/suppress hook without claiming unproven Raspberry Pi reset or boot-recovery behavior.

## What Changes

- Add a new `WatchdogSupervisor` core component as the single active-baseline owner for software watchdog and runtime liveness supervision.
- Absorb the current `HealthMonitor` CPU/RSS resource-monitoring behavior into `WatchdogSupervisor` and retire the separate `HealthMonitor` owner.
- Keep the existing resource-monitoring commands/events/telemetry behavior, but migrate that public surface to `OBCApp.watchdogSupervisor` as the new owner.
- Supervise a bounded fast-rate-group source set in v1: `EpsBridge`, `EpsFdirController`, `ModeSafetyController`, and `CommController`.
- Use explicit per-source heartbeat beats from those owners rather than inferred schedule freshness or upstream active-component ping alone.
- Escalate deterministically from source-stale warning to latched watchdog fault, at-most-one watchdog `SAFE` request per fault epoch once the current mode is requestable, and supervisor-side watchdog-feed suppression eligibility.
- Add bounded watchdog public surfaces aligned with the target design where the current baseline can support them: `GET_WATCHDOG_STATUS` and `SET_WATCHDOG_CONFIG`.
- Add focused component/helper coverage, hosted probe evidence, and verification-path registry updates for healthy, stale, escalated, feed-suppressed, and recovered watchdog behavior.
- **BREAKING**: the resource-monitoring owner instance name and command path move from `healthMonitor` to `watchdogSupervisor`; the old `HealthMonitor` component is removed rather than left as a compatibility owner.

## Capabilities

### New Capabilities

- none

### Modified Capabilities

- `core-system-contracts`: replace `HealthMonitor` with `WatchdogSupervisor` as the concrete core owner for resource-monitoring and watchdog public surfaces.
- `mission-autonomy`: add a separate `WatchdogSupervisor` owner for liveness supervision and deterministic mode/fault escalation while keeping `ModeSafetyController` and `EpsFdirController` narrow.
- `verification-evidence`: require reviewable local evidence for software watchdog supervision, escalation, feed eligibility/suppression, recovery clear behavior, and resource-monitoring migration truth.
- `verification-path-registry`: register the hosted watchdog-v1 path as distinct from EPS timeout FDIR, target watchdog reset, and boot recovery.

## Impact

- Affected code: new `OBC/Components/WatchdogSupervisor/*`; removal or migration of `OBC/Components/HealthMonitor/*`; supervised-source heartbeat ports and runtime wiring in `EpsBridge`, `EpsFdirController`, `ModeSafetyController`, and `CommController`; active topology and hosted runtime status/reporting paths.
- Affected public/runtime surfaces: `GET_WATCHDOG_STATUS`, `SET_WATCHDOG_CONFIG`, migrated `HEALTH_*` resource-monitoring commands, migrated `SYS_*` resource-monitoring telemetry/events, new watchdog events/telemetry, and a distinct internal mode-apply source for watchdog fault escalation.
- Affected tests and probes: new classic F' L2 watchdog coverage, direct helper tests, updated resource-monitoring tests, and a new repository-owned hosted watchdog probe.
- Affected docs/evidence: new `evidence/records/watchdog-v1/README.md`, verification-path registry updates, and active-baseline architecture/roadmap truth updates.
