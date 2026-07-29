## Why

`RecoveryExecutor` already records bounded R2 process-restart intent, and the Raspberry Pi target already has service-managed restart/autostart evidence. The remaining gap is that current R2 recovery sources do not yet execute a truthful managed OBC process restart through that target service model.

## What Changes

- Convert current R2 recovery sources from intent-only reporting into a real managed OBC process/service restart.
- Keep R6 as the existing reboot-equivalent runtime exit path and keep R2 distinct from Linux or hardware reboot.
- Persist R2 recovery metadata before runtime exit so the restarted process reloads `reset_cause`, `last_recovery_source`, and `last_recovery_level=R2_RESTART_SOFTWARE_COMPONENT`.
- Split process-restart runtime state from reboot state in status, telemetry, tests, and probes.
- Add hosted and Raspberry Pi target evidence for R2 restart closure while leaving hardware watchdog reset, bootloader/partition handoff, and power-loss recovery outside this change.
- Update roadmap, architecture, interface, verification registry, and test-record documentation to reflect the narrowed remaining target-recovery scope.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `mission-autonomy`: `RecoveryExecutor` R2 action semantics change from intent-only to real managed process restart for watchdog and ADCS poll FDIR sources.
- `boot-update`: boot metadata shall preserve recovery-triggered process restart truth as distinct from R6 reboot truth.
- `platform-baseline`: the Raspberry Pi service-managed target baseline shall be reusable for R2 OBC process restart closure without claiming hardware reset or power-loss behavior.
- `verification-evidence`: evidence requirements shall include hosted and Raspberry Pi proof for R2 process restart closure.
- `verification-path-registry`: the registry shall record the newly proven hosted and target R2 restart paths separately from hardware watchdog reset and power-loss paths.

## Impact

- Affected code includes `RecoveryExecutor`, `BootManager` runtime recovery metadata handoff, hosted runtime exit handling, related FPP type/status surfaces, and unit tests.
- Affected probes include the recovery-executor, multi-subsystem FDIR, watchdog/persistent-ring paths as needed, plus a new target R2 restart probe.
- Affected documents include active architecture, roadmap, interface contract notes, verification path registry, and a new `docs/test-records/target-recovery-closure-v1/` evidence record.
