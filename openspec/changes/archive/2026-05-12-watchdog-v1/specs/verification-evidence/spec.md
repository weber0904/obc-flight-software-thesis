## ADDED Requirements

### Requirement: Watchdog-v1 Evidence Is Reviewable

The verification evidence SHALL record reviewable local evidence for `watchdog-v1`, including focused component/helper coverage, hosted probe evidence, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover watchdog thresholds and recovery

- **WHEN** `watchdog-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering:
  - migrated CPU/RSS resource-monitoring behavior under the new owner
  - healthy heartbeat behavior for all supervised sources
  - warning-only threshold crossing
  - fault latch without overstating `SAFE_REQUESTED` before a real watchdog `SAFE` request
  - at-most-one watchdog `SAFE` escalation once the current mode is requestable
  - no duplicate `SAFE` request while watchdog fault remains latched
  - continued stale progression to feed suppression
  - first-beat source recovery clear
  - aggregate recovery waiting for all enabled supervised sources

#### Scenario: Integration evidence uses the normal mode surface

- **WHEN** `watchdog-v1` records integration evidence
- **THEN** the evidence SHALL show that watchdog fault escalation requests go through `ModeManager::applyModeForInternalSource` or the equivalent normal runtime mode-control path
- **AND** it SHALL show that the outward-facing mode result remains the normal `SYS_MODE_CHANGE` surface rather than a parallel watchdog-owned mode store

#### Scenario: Hosted probe evidence is reviewable

- **WHEN** `watchdog-v1` records hosted evidence
- **THEN** the evidence SHALL include the hosted probe command, isolated runtime roots or ports, the bounded beat-suppression method used to create stale watchdog sources, expected outcomes for healthy, warning, latched, suppressed, and recovered cases, observed outcomes, and final verdict

#### Scenario: Evidence record captures closeout commands

- **WHEN** `watchdog-v1` is ready for closeout
- **THEN** the evidence SHALL include:
  - `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-watchdog-v1`
  - focused `ctest` commands for affected watchdog, mission-autonomy, and migrated resource-monitoring coverage
  - `bash scripts/run_watchdog_v1_probe.sh`
  - `openspec validate watchdog-v1`
  - `openspec validate --specs`

#### Scenario: Exclusions stay explicit

- **WHEN** `watchdog-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove Raspberry Pi hardware watchdog reset, boot-safe-image recovery, reset-cause persistence, process restart executors, subsystem reset executors, broad multi-subsystem FDIR, persistent fault/event storage, RF behavior, target hardware closure, or a broader system-supervisor framework
