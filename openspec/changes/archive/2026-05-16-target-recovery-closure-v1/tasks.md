## 1. Runtime Recovery Semantics

- [x] 1.1 Add active `PROCESS_RESTART` action vocabulary and process-restart status/telemetry distinct from reboot status.
- [x] 1.2 Change `RecoveryExecutor` R2 handling so watchdog and ADCS R2 sources persist metadata and queue process restart instead of intent-only reporting.
- [x] 1.3 Add restart-loop guard using boot safe-fallback state and keep R6 reboot-equivalent behavior distinct.
- [x] 1.4 Update hosted runtime exit handling to consume typed recovery exits and return `31` for R2 and `32` for R6.

## 2. Boot Metadata And Tests

- [x] 2.1 Generalize BootManager runtime recovery-intent persistence for R2 process restart and R6 reboot metadata.
- [x] 2.2 Add/update RecoveryExecutor unit tests for R2 persistence, status, action reporting, and loop clamp.
- [x] 2.3 Add/update BootManager and hosted-runtime tests for R2 metadata reload and R2/R6 exit-code separation.

## 3. Probes And Target Evidence

- [x] 3.1 Update hosted recovery/multi-subsystem/watchdog/persistent-ring probes to assert R2 real process restart where applicable.
- [x] 3.2 Add a Raspberry Pi target probe that induces ADCS R2 by stopping the ADCS subsystem service, verifies systemd OBC restart, restores ADCS service, and reads back boot metadata.
- [x] 3.3 Record `docs/test-records/target-recovery-closure-v1/` evidence and update `docs/verification-path-registry.md`.

## 4. Documentation And OpenSpec Closeout

- [x] 4.1 Update README, current architecture, roadmap, and interface docs for the new R2 closure and remaining hardware-reset boundaries.
- [x] 4.2 Run focused tests/probes, the shared verification gate, `openspec validate target-recovery-closure-v1`, and `openspec validate --specs`.
- [x] 4.3 Archive the OpenSpec change, update reconciliation matrix artifacts, and run repository consistency checks.
- [ ] 4.4 Commit, push, open a ready PR, wait for CI green, merge, and sync local `main`.
