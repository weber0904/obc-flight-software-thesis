## ADDED Requirements

### Requirement: Lab Serial Ingress Evidence Is Staged
The verification evidence tree SHALL record physical lab serial ingress evidence with separate Stage 0 prerequisite, Stage 1 uplink ingress, and Stage 2 downlink/full-TT&C verdicts.

#### Scenario: Uplink ingress evidence is reviewable
- **WHEN** the staged lab serial ingress probe completes Stage 1
- **THEN** reviewers SHALL be able to inspect the host serial device, subsystem serial device, baudrate, GDS ports, CSP hub ports, gateway launch, remote COMM node launch, OBC launch mode, commands issued, and OBC readback used for the verdict
- **AND** when the probe uses a gateway serial acquisition preamble or bounded command retries, the evidence SHALL record the preamble settings and retry bound

#### Scenario: Downlink evidence is not implied by uplink
- **WHEN** Stage 1 passes but Stage 2 fails or is inconclusive
- **THEN** the evidence SHALL state that only physical lab serial uplink ingress was proven
- **AND** it SHALL keep `fprime-cli events`, `fprime-cli channels`, file/downlink, RF, target OBC, and COMM shared CAN FD outside the proven verdict

#### Scenario: Full TT&C evidence requires both directions
- **WHEN** the evidence describes the physical lab serial result as bounded TT&C
- **THEN** it SHALL include both OBC command readback and ground-side event/telemetry observations
