## ADDED Requirements

### Requirement: Recovery Executor Evidence Is Reviewable
The verification evidence SHALL record reviewable local evidence for `recovery-executors-v1`, including focused component/helper coverage, shared-detector integration coverage, hosted reboot-equivalent probe evidence, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover shared recovery progression
- **WHEN** `recovery-executors-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering:
  - shared incident open, status, relatch, and clear behavior
  - watchdog detector request/clear emission into the shared executor path
  - EPS detector request/clear emission into the shared executor path
  - reviewable process-restart intent recording without over-claiming a real process restart
  - real EPS interface reset execution
  - one-shot `SAFE` fallback through the normal mode-control surface
  - reboot escalation on repeated failure
  - boot metadata schema upgrade, reset-cause truth, boot-count truth, repeated-reset safe clamp, and stable-ack clearing

#### Scenario: Integration evidence keeps SAFE on the normal mode path
- **WHEN** `recovery-executors-v1` records integration evidence
- **THEN** the evidence SHALL show that shared recovery `SAFE` fallback still goes through `ModeManager::applyModeForInternalSource` or the equivalent normal runtime mode-control path
- **AND** it SHALL show that the outward-facing mode result remains the normal `SYS_MODE_CHANGE` surface rather than a parallel recovery-owned mode store

#### Scenario: Hosted probe evidence proves reboot-equivalent closure
- **WHEN** `recovery-executors-v1` records hosted evidence
- **THEN** the evidence SHALL include the hosted probe command, isolated runtime roots or ports, bounded stale/timeout injection method, same-runtime-root relaunch method, expected outcomes for watchdog progression, EPS progression, reboot relaunch truth, and final verdict

#### Scenario: Evidence record captures closeout commands
- **WHEN** `recovery-executors-v1` is ready for closeout
- **THEN** the evidence SHALL include:
  - the fresh local verification build command used for the change
  - focused `ctest` commands for affected recovery, watchdog, EPS, boot, and snapshot coverage
  - `bash scripts/run_recovery_executors_v1_probe.sh`
  - `openspec validate recovery-executors-v1`
  - `openspec validate --specs`

#### Scenario: Exclusions stay explicit
- **WHEN** `recovery-executors-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove Raspberry Pi hardware watchdog reset, target hardware reboot, subsystem power-cycle action, broad all-subsystem FDIR, persistent event-log infrastructure, TTC/pass scheduling, payload autonomy, or full secure boot redesign
