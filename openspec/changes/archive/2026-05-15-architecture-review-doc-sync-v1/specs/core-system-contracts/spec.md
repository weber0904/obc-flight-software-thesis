## MODIFIED Requirements

### Requirement: UHF Backup Command Policy Is Conservative In V1
The v1 command authority policy SHALL allow UHF backup role to execute only the
explicit read/status command allowlist and SHALL reject other active command
classes until later governed authority changes expand that role.

#### Scenario: UHF backup allows only read/status commands
- **WHEN** a command is evaluated under `UHF + BACKUP`
- **THEN** only current-mode, EPS status, ADCS attitude, GPS state, radio status, storage status, and boot status commands SHALL be allowed
- **AND** mode changes, ADCS control, EPS writes or resets, radio configuration, COMM active/pass control, boot update/reset operations, current file/downlink commands, retired housekeeping file/downlink commands if present in legacy dictionaries, sequencer commands, CSP commands, configuration updates, payload control, and unclassified commands SHALL be denied.
