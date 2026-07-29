## Context

`RecoveryExecutor` currently normalizes watchdog and ADCS recovery sources to `R2_RESTART_SOFTWARE_COMPONENT`, but the active action is still `PROCESS_RESTART_INTENT`. R6 already persists recovery metadata through `BootManager`, causes the hosted runtime to exit with a nonzero reboot-equivalent code, and relies on the managed launch path to restart the OBC process.

The Raspberry Pi target baseline already proves the active `TopCcsds` `OBC` package, systemd-managed startup, target process restart/autostart, boot metadata reload, and command-freshness persistence. This change connects `RecoveryExecutor` R2 to that existing managed restart plane without claiming hardware watchdog reset, Linux reboot, bootloader/partition handoff, or power-loss recovery.

## Goals / Non-Goals

**Goals:**

- Make all current R2 sources execute a real managed OBC process restart after persistent recovery metadata is recorded.
- Keep R2 process restart distinct from R6 reboot-equivalent exit in status, telemetry, events, probes, and evidence.
- Preserve existing boot metadata compatibility while making `lastRecoveryLevel=R2_RESTART_SOFTWARE_COMPONENT` observable after relaunch.
- Add hosted and Raspberry Pi evidence that proves the R2 restart path and documents remaining hardware-reset gaps.

**Non-Goals:**

- No Raspberry Pi hardware watchdog stroking or `/dev/watchdog` reset proof.
- No target power-loss recovery, bootloader, partition handoff, or final flight deployment claim.
- No new probe-only public command surface unless the service-fault target trigger proves infeasible.
- No legacy `OBC/Top` retirement in this change.

## Decisions

- Add an active `PROCESS_RESTART` recovery action and keep `PROCESS_RESTART_INTENT` only as historical compatibility vocabulary. Active R2 code, tests, and evidence shall use `PROCESS_RESTART`.
- Split runtime exit handling into a typed request: `NONE`, `PROCESS_RESTART`, and `OBC_REBOOT`. Hosted/runtime entrypoints shall return exit code `31` for R2 process restart and preserve exit code `32` for R6 reboot-equivalent.
- Generalize the BootManager runtime recovery intent API so R2 restart and R6 reboot can both persist metadata before exit. The persisted schema may keep existing field names for compatibility; level and action semantics disambiguate R2 from R6.
- Treat watchdog stale faults and ADCS poll transport/freshness faults as the current R2 sources. Each source uses the same metadata-before-exit path.
- Guard restart loops by checking boot safe-fallback state before issuing another R2 exit. When the repeated-recovery clamp is active, the executor records the incident and holds/request safe fallback instead of queueing another process restart.
- Prove target closure through the existing lab COMM CSP/service-managed path by stopping the ADCS subsystem service to trigger a real scheduled-poll ADCS R2 fault. This avoids adding a probe-only command and exercises the systemd restart boundary that will run in the target profile.

## Risks / Trade-offs

- Target probe stability depends on remote services and CAN/CSP lab setup. Mitigation: keep the probe bounded, restore the ADCS service in cleanup, and record setup failures separately from product failures.
- Reusing boot metadata fields with historical reset naming can be confusing. Mitigation: status/docs shall explicitly identify `R2_RESTART_SOFTWARE_COMPONENT` as process restart and keep R6-specific reboot counters separate.
- R2 immediate exit changes hosted probe timing. Mitigation: update affected probes and assert the new exit code and post-relaunch metadata instead of old intent-only observations.
- Repeated-fault behavior moves from in-memory relatch-only progression to boot-safe-fallback guarded restart prevention for R2 loops. Mitigation: add unit tests for the clamp and preserve R6 escalation behavior for explicit reboot-equivalent paths.
