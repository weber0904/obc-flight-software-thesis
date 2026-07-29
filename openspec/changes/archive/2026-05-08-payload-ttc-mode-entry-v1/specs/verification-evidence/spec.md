## ADDED Requirements

### Requirement: Payload TTC Mode Entry Evidence
The verification evidence SHALL record reviewable local evidence for payload-ttc-mode-entry-v1, including OpenSpec validation, focused component/helper tests, hosted shell regression, default hosted CCSDS S-band command/event/telemetry coverage, and explicit deferred boundaries.

#### Scenario: Component and helper evidence covers the matrix
- **WHEN** payload-ttc-mode-entry-v1 completes local verification
- **THEN** the evidence SHALL identify focused tests covering all 25 operator from/to mode pairs, same-mode no-op side effects, rejected transition no-change semantics, rejection reason mapping, guard-unconfigured response mapping, generated invalid-enum `FORMAT_ERROR` behavior, parser invalid spelling behavior, and internal safety apply path separation

#### Scenario: HELL recovery guard evidence is reviewable
- **WHEN** payload-ttc-mode-entry-v1 records guard evidence
- **THEN** the evidence SHALL identify tests covering operator `HELL -> SAFE` with cached EPS SoC greater than `15%`, exactly `15%`, less than `15%`, and unavailable cache
- **AND** the evidence SHALL identify tests or probes showing autonomous `ModeSafetyController` fallback and recovery still use the existing cached-EPS policy

#### Scenario: Hosted shell regression evidence is reviewable
- **WHEN** payload-ttc-mode-entry-v1 records hosted shell evidence
- **THEN** the evidence SHALL include the hosted shell probe command, isolated runtime root or ports, boot-mode precondition check for `SYS_MODE == SAFE`, bounded command sequence, command outcomes, final mode observations, parser rejection cases, and final verdict

#### Scenario: Default hosted CCSDS S-band command path evidence is reviewable
- **WHEN** payload-ttc-mode-entry-v1 records hosted command path evidence
- **THEN** the evidence SHALL use the existing default hosted CCSDS S-band OBC path through `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> sband_comm_csp_node(node 5) -> CSP -> OBC`
- **AND** it SHALL identify bounded `MODE_SET` commands, observed command responses, mode-change or rejection events, final `SYS_MODE` telemetry, and the reused verification registry path
- **AND** it SHALL NOT depend on event and telemetry transport arrival ordering

#### Scenario: Evidence record has reproducibility fields
- **WHEN** payload-ttc-mode-entry-v1 records final evidence
- **THEN** the evidence SHALL include branch name, base commit SHA, final local commit SHA, OpenSpec change name, exact build and verification commands, unit/component test result summary, hosted shell sequence and results, CCSDS/F Prime command sequence and results, command responses, event excerpts, `SYS_MODE` before/after telemetry snapshots, reused or new verification path, and deferred work

#### Scenario: Exclusions stay explicit
- **WHEN** payload-ttc-mode-entry-v1 evidence is recorded
- **THEN** the evidence SHALL state that it does not prove scheduler/pass-window automation, ADCS ground tracking, payload/camera control, payload power sequencing, link authority, auth/session policy, UHF failover, CCSDS route changes, HK data-product field changes, watchdog behavior, subsystem timeout/retry/reset behavior, broader FDIR, RF behavior, target hardware, or real payload/TTC mission execution

#### Scenario: OpenSpec validation is recorded
- **WHEN** payload-ttc-mode-entry-v1 is ready for closeout
- **THEN** the evidence SHALL include `openspec validate payload-ttc-mode-entry-v1` and `openspec validate --specs` results
