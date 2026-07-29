## ADDED Requirements

### Requirement: TTC Pass-Window Mode Evidence
The verification evidence SHALL record reviewable local and hosted proof for ttc-pass-window-mode-v1, including focused helper/component tests, hosted TTC policy proof, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Helper and component evidence is reviewable
- **WHEN** ttc-pass-window-mode-v1 completes local verification
- **THEN** the evidence SHALL identify tests covering epoch-window validation, window-active comparison, UTC-to-epoch conversion, GPS freshness validity, COMM loss timeout behavior, TTC config/window command handling, policy status semantics, auto-entry, auto-exit, manual coexistence, and safety/recovery precedence interactions

#### Scenario: Hosted TTC pass-window probe evidence is reviewable
- **WHEN** ttc-pass-window-mode-v1 records hosted proof
- **THEN** the evidence SHALL include the hosted probe command, isolated runtime roots or ports, bounded GPS/COMM setup method, expected outcomes, observed outcomes, and final verdict
- **AND** it SHALL identify proof for:
  - no-entry when TTC is disabled
  - auto-entry when TTC is enabled and the active pass window is valid
  - auto-exit when the pass window ends
  - safety precedence during TTC
  - manual and automatic TTC coexistence without split ownership
  - truthful hosted COMM-loss behavior on the chosen hosted path, including whether shared recovery precedence prevents direct observation of TTC policy timeout exit

#### Scenario: Evidence record has reproducibility fields
- **WHEN** ttc-pass-window-mode-v1 records final evidence
- **THEN** the evidence SHALL include branch name, base commit SHA, final local commit SHA, OpenSpec change name, exact build and verification commands, focused test summary, hosted probe sequence and results, command/status excerpts, reused or new verification path, and deferred work

#### Scenario: Exclusions stay explicit
- **WHEN** ttc-pass-window-mode-v1 evidence is recorded
- **THEN** it SHALL state that the evidence does not prove generic scheduler behavior, TLE upload/parsing, orbital propagation, ADCS ground tracking, payload scheduling, COMM RF lock closure, target hardware behavior, Raspberry Pi deployment closure, or a broader mission-planning framework

#### Scenario: OpenSpec validation is recorded
- **WHEN** ttc-pass-window-mode-v1 is ready for closeout
- **THEN** the evidence SHALL include `openspec validate ttc-pass-window-mode-v1` and `openspec validate --specs` results
