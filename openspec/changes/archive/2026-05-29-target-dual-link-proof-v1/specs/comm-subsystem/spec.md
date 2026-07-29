## ADDED Requirements

### Requirement: First Target-Bearing Dual-Link Proof Is Branch-Scoped And Switch-Closed

The comm subsystem SHALL implement the first repository-owned target-bearing
dual-link proof on the physical target CAN + UHF UART path as a bounded,
primary-led, switch-closed family.

#### Scenario: Default node-5 truth remains the governing primary surface
- **WHEN** the first implementation-bearing target dual-link proof runs
- **THEN** it SHALL close phase A on the default target node-`5` S-band
  command truth
- **AND** it SHALL NOT treat node-`6` activity as replacing that governing
  primary surface

#### Scenario: Phase B adjunct stays minimal and role-valid
- **WHEN** the same proof exercises concurrent non-quiet node-`6`
  `uhf-backup`
- **THEN** its mandatory adjunct gate SHALL be one allowlisted low-authority
  read/status command
- **AND** it SHALL NOT require a richer same-path `EPS_GET_STATUS` plus
  `ADCS_GET_ATTITUDE` sequence as the mandatory phase-B PASS gate

#### Scenario: Phase C remains independently non-quiet
- **WHEN** the proof claims formal UHF command truth
- **THEN** it SHALL close that truth only after explicit switch to
  `uhf-primary-after-failover`
- **AND** that switched phase SHALL pass on the non-quiet physical node-`6`
  path even if quiet rescue was needed earlier for phase B

#### Scenario: Quiet rescue stays phase-B-only
- **WHEN** a failed non-quiet `uhf-backup` adjunct attempt is retried on quiet
  node-`6`
- **THEN** the repository MAY use that retry only to rescue the phase-B
  adjunct
- **AND** it SHALL NOT describe that rescue as replacing the mandatory phase-C
  switched non-quiet truth
