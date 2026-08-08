## ADDED Requirements

### Requirement: Mode SoC Admission And Exit Evidence
The verification evidence SHALL record reviewable local evidence for `mode-soc-admission-and-exit-v1`, including focused helper/component/integration coverage, hosted shell regression, default hosted CCSDS S-band command-path coverage, OpenSpec validation, and explicit deferred boundaries.

#### Scenario: Focused tests cover SoC admissions and payload exit
- **WHEN** `mode-soc-admission-and-exit-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering:
  - operator `SAFE -> IDLE` accepted only when cached SoC is strictly greater than `50%`
  - operator `IDLE -> PAYLOAD` accepted only when cached SoC is strictly greater than `70%`
  - unavailable cached EPS fail-closed behavior for SoC-guarded admits
  - automatic `PAYLOAD -> IDLE` when cached SoC is less than `60%`
  - `PAYLOAD -> SAFE` below `40%` taking precedence over `PAYLOAD -> IDLE`
  - unchanged `IDLE -> TTC` behavior without a new SoC admission threshold

#### Scenario: Hosted shell evidence is reviewable
- **WHEN** `mode-soc-admission-and-exit-v1` records hosted shell evidence
- **THEN** the evidence SHALL include the shell probe command, isolated runtime root or ports, bounded operator sequence, command outcomes, rejection reasons, final mode observations, and final verdict

#### Scenario: Default hosted CCSDS S-band MODE_SET evidence is reviewable
- **WHEN** `mode-soc-admission-and-exit-v1` records hosted command-path evidence
- **THEN** the evidence SHALL use the existing default hosted CCSDS S-band OBC path through `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> CSP -> OBC`
- **AND** it SHALL identify bounded `MODE_SET` commands, observed command responses, mode-change or rejection events, final `SYS_MODE` telemetry, and the reused verification registry path
- **AND** it SHALL include at least one fail-closed unavailable-cache case for `SAFE -> IDLE` or `IDLE -> PAYLOAD`

#### Scenario: Evidence record captures closeout commands
- **WHEN** `mode-soc-admission-and-exit-v1` is ready for closeout
- **THEN** the evidence SHALL include:
  - fresh build and affected test commands
  - focused hosted probe commands
  - `openspec validate mode-soc-admission-and-exit-v1`
  - `openspec validate --specs`
  - `python3 scripts/check_repo_consistency.py`

#### Scenario: Exclusions stay explicit
- **WHEN** `mode-soc-admission-and-exit-v1` evidence is recorded
- **THEN** the evidence SHALL state that it does not prove TTC pass scheduling, TLE/GPS/pass-window logic, ADCS ground tracking, payload/camera control, payload power sequencing, watchdog behavior, subsystem timeout/retry/reset FDIR, command auth/session lifecycle, configurable SoC threshold commands, RF behavior, target hardware, or a new mode engine
