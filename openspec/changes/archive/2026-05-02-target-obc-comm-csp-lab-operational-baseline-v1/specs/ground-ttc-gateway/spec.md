## ADDED Requirements

### Requirement: Lab Target Ground Launcher
The ground TT&C gateway SHALL provide a repo-owned launcher for the lab target COMM CSP operational path that starts stock `fprime-gds` and `ground_ttc_gateway` with reviewable operator settings.

#### Scenario: Ground launcher records operator-facing settings
- **WHEN** the lab target ground launcher starts
- **THEN** it SHALL print the GDS bind address, GDS IP port, GDS TTS port, GDS file-storage directory, gateway serial endpoint, baudrate, preamble line count, and preamble delay
- **AND** it SHALL keep stock F' framing toward `fprime-gds`

#### Scenario: Ground launcher stays a lab operator surface
- **WHEN** the launcher is documented or used as evidence
- **THEN** it SHALL be described as the macOS lab ground surface for the RF-omitted path
- **AND** it SHALL NOT claim RF validation or a custom GDS communication plugin

### Requirement: Lab Target Operator Runbook
The ground TT&C gateway capability SHALL include an operator runbook for the lab target COMM CSP path.

#### Scenario: Runbook describes complete lab operations
- **WHEN** an operator follows the runbook
- **THEN** it SHALL cover OBC package/install, OBC service migration, CAN provisioning, subsystem services, ground launcher startup, status and journal inspection, rollback to the older installed service, and E2E command/file verification

#### Scenario: Runbook preserves proof boundaries
- **WHEN** the runbook describes expected results
- **THEN** it SHALL distinguish lab operational readiness from final flight deployment
- **AND** it SHALL keep RF, no-preamble first-byte-clean behavior, reliable retransmission, arbitrary file downlink, ScenarioBridge/pass automation, and final OS CAN provisioning out of scope
