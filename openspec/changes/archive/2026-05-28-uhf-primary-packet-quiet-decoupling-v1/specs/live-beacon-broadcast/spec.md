## MODIFIED Requirements

### Requirement: Beacon Suppress And Resume Policy Is COMM-Owned

The current baseline SHALL treat UHF beacon suppress and resume as COMM runtime
owned by `CommController`, SHALL keep suppress start tied to accepted
authenticated qualifying UHF `SESSION_OPEN(seq0)` and later accepted
same-session UHF activity, and SHALL describe formal UHF primary packet quiet
as a separate current-primary-band cleanliness rule rather than as the beacon
owner itself.

#### Scenario: Backup role is not rewritten as beacon-only
- **WHEN** current baseline wording describes UHF backup behavior
- **THEN** it SHALL keep beacon duty separate from the bounded allowlisted
  backup-ingress command surface
- **AND** it SHALL NOT describe `uhf-backup` as beacon-only

#### Scenario: Entering UHF primary alone does not suppress beacon
- **WHEN** the current baseline describes UHF primary clean-path behavior
- **THEN** it SHALL state that entering UHF primary alone does not start beacon
  suppress
- **AND** it SHALL keep beacon suppress start scoped to accepted qualifying UHF
  `SESSION_OPEN(seq0)` ownership

#### Scenario: Active qualifying UHF session still suppresses beacon chatter only in that window
- **WHEN** the current baseline describes UHF beacon suppress/runtime behavior
- **THEN** it SHALL state that active qualifying UHF session windows suppress
  beacon chatter
- **AND** it SHALL keep that statement separate from the independently active
  UHF primary packet quiet rule
