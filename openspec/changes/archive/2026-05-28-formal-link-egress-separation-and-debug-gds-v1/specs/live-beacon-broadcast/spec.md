## MODIFIED Requirements

### Requirement: Beacon Suppress And Resume Policy Is COMM-Owned

The current baseline SHALL treat UHF beacon suppress and resume as COMM runtime
owned by `CommController`, and active UHF command-session suppression SHALL be
described as a clean-session rule for `uhf-primary-after-failover` rather than
as a redefinition of `uhf-backup` or of official file/downlink behavior.

#### Scenario: Backup role is not rewritten as beacon-only
- **WHEN** current baseline wording describes UHF backup behavior
- **THEN** it SHALL keep beacon duty separate from the bounded allowlisted
  backup-ingress command surface
- **AND** it SHALL NOT describe `uhf-backup` as beacon-only

#### Scenario: Active UHF primary session suppresses beacon chatter only in that window
- **WHEN** the current baseline describes UHF primary clean-session behavior
- **THEN** it SHALL state that active UHF command-session windows suppress
  beacon chatter
- **AND** it SHALL keep that statement scoped to the active UHF
  command-session window rather than as a blanket permanent UHF beacon disable
