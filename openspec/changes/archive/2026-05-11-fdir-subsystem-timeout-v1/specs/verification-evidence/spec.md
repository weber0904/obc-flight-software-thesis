## ADDED Requirements

### Requirement: EPS Timeout FDIR Evidence
The verification evidence SHALL record reviewable local evidence for `fdir-subsystem-timeout-v1`, including focused component/helper/integration coverage, hosted probe evidence, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover timeout thresholding and recovery
- **WHEN** `fdir-subsystem-timeout-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering:
  - EPS consecutive poll-failure counting
  - cumulative EPS poll comm-error counting
  - reset-to-healthy behavior on the first successful poll
  - no escalation on failures `1` and `2`
  - one-shot escalation at failure `3`
  - no duplicate escalation while the fault remains latched
  - recovery clear on the first successful poll after fault
  - `SAFE` and `HELL` fault-only behavior with no duplicate mode request

#### Scenario: Integration evidence uses the normal mode surface
- **WHEN** `fdir-subsystem-timeout-v1` records integration evidence
- **THEN** the evidence SHALL show that EPS timeout escalation requests go through `ModeManager::applyModeForInternalSource` or the equivalent normal runtime mode-control path
- **AND** it SHALL show that the outward-facing mode result remains the normal `SYS_MODE_CHANGE` surface rather than a parallel fault-owned mode store

#### Scenario: Hosted probe evidence is reviewable
- **WHEN** `fdir-subsystem-timeout-v1` records hosted evidence
- **THEN** the evidence SHALL include the hosted probe command, isolated runtime roots or ports, simulator stop or restart method, expected outcomes for transient-failure, threshold-crossing, and recovery-after-fault cases, observed outcomes, and final verdict

#### Scenario: Evidence record captures closeout commands
- **WHEN** `fdir-subsystem-timeout-v1` is ready for closeout
- **THEN** the evidence SHALL include:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-fdir-subsystem-timeout-v1`
  - focused `ctest` commands for affected EPS and mission-autonomy coverage
  - `bash scripts/run_eps_csp_integration.sh`
  - `bash scripts/run_eps_timeout_fdir_hosted_probe.sh`
  - `openspec validate fdir-subsystem-timeout-v1`
  - `openspec validate --specs`

#### Scenario: Exclusions stay explicit
- **WHEN** `fdir-subsystem-timeout-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove timestamp freshness policy, EPS reset or power-cycle recovery, multi-subsystem FDIR, watchdog behavior, persistent fault/event storage, CCSDS command-path expansion, RF behavior, target hardware, command auth/session/QoS work, TTC pass automation, payload control, or a broader mission-execution framework
