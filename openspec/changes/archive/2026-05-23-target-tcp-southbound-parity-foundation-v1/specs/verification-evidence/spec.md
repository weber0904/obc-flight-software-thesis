## ADDED Requirements

### Requirement: Target TCP Parity Topology Is Explicit

Target TCP matrix evidence SHALL identify the governed three-host parity
topology so later target TCP cells reuse the same launch boundary.

#### Scenario: Three-host target TCP topology is reviewable
- **WHEN** target TCP matrix evidence is recorded
- **THEN** reviewers SHALL be able to identify the macOS ground stack, the
  OBC-only process on `obc.local`, and the COMM plus subsystem services on
  `subsystem.local`

### Requirement: Target TCP UHF Evidence Stays Development-Carrier Scoped

Target TCP node-`6` evidence SHALL remain explicitly scoped to TCP-based
southbound emulation.

#### Scenario: Target TCP UHF proof is not mistaken for physical UART
- **WHEN** target TCP `uhf-primary-*` cells pass
- **THEN** their evidence SHALL identify TCP-based southbound emulation
- **AND** it SHALL NOT describe those results as target physical-UHF closure
