## ADDED Requirements

### Requirement: Command Authority Vocabulary Is Defined
The core system contract SHALL define a reusable command authority vocabulary for routed F Prime command packets, including link identity, configured link role, command class, resource label, authority decision, and rejection reason.

#### Scenario: Vocabulary separates identity from role
- **WHEN** command authority policy is described
- **THEN** link identity such as `SBAND`, `UHF`, `DEV_DIRECT`, `INTERNAL`, or `UNKNOWN` SHALL remain distinct from configured role such as `PRIMARY`, `BACKUP`, `PRIMARY_AFTER_FAILOVER`, `DEV_FULL`, or `INTERNAL`
- **AND** the vocabulary SHALL NOT hard-code `UHF` as permanently low authority or `SBAND` as permanently primary.

#### Scenario: Vocabulary does not claim runtime enforcement
- **WHEN** `link-authority-vocabulary-v1` is cited
- **THEN** it SHALL be cited only as vocabulary and policy-source definition
- **AND** it SHALL NOT be cited as runtime command ingress enforcement, full link authority, file authority, uplink authority, crypto authentication, session sequencing, replay protection, or failover enforcement.

### Requirement: Command Authority Policy Uses Fully Qualified Command Names
The command authority policy source SHALL key command classification by fully qualified FPP JSON dictionary command names and SHALL derive runtime opcode lookup from dictionary data.

#### Scenario: Short command names are not policy keys
- **WHEN** policy classifies commands from the active topology dictionaries
- **THEN** it SHALL use `commands[].name` values such as `OBCApp.modeManager.MODE_GET`
- **AND** it SHALL NOT rely on suffix-only names such as `GET_STATUS`
- **AND** it SHALL NOT hand-maintain global opcode numbers as the policy source of truth.

#### Scenario: Every active command is classified
- **WHEN** repository tests inspect the default CCSDS and legacy ComFprime topology dictionaries
- **THEN** every command entry SHALL have a command authority classification
- **AND** missing classifications SHALL fail the focused policy/catalog tests.

### Requirement: UHF Backup Command Policy Is Conservative In V1
The v1 command authority policy SHALL allow UHF backup role to execute only the explicit read/status command allowlist and SHALL reject other active command classes until later governed authority changes expand that role.

#### Scenario: UHF backup allows only read/status commands
- **WHEN** a command is evaluated under `UHF + BACKUP`
- **THEN** only current-mode, EPS status, ADCS attitude, GPS state, radio status, storage status, and boot status commands SHALL be allowed
- **AND** mode changes, ADCS control, EPS writes or resets, radio configuration, COMM active/pass control, boot update/reset operations, housekeeping file/downlink commands, CSP commands, configuration updates, payload control, and unclassified commands SHALL be denied.

#### Scenario: Future sequencer commands require explicit classification
- **WHEN** a future topology adds command sequencer load, run, or control commands
- **THEN** those commands SHALL require explicit authority classification
- **AND** UHF backup policy SHALL deny them unless a later governed sequence-authority model explicitly allows them.
