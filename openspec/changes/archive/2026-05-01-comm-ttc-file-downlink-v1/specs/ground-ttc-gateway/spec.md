## ADDED Requirements

### Requirement: Gateway-Backed COMM File Downlink Validation
The ground TT&C gateway SHALL support governed file/downlink validation where stock F' file downlink traffic traverses the existing gateway-backed COMM path and lands in the configured GDS file-storage directory.

#### Scenario: File proof follows bounded TT&C readiness
- **WHEN** the physical lab serial COMM file/downlink probe runs
- **THEN** it SHALL first verify gateway-backed command/event/channel TT&C readiness before claiming file/downlink success

#### Scenario: Ground storage receives bounded archive files
- **WHEN** `HK_DOWNLINK_INDEX` and `HK_DOWNLINK_SLOT` are sent through the gateway-backed COMM path
- **THEN** the configured GDS file-storage directory SHALL receive the expected housekeeping index and selected slot files

#### Scenario: Gateway file evidence records transport settings
- **WHEN** the gateway-backed COMM file/downlink evidence is recorded
- **THEN** it SHALL record the gateway transport, serial endpoint settings, preamble settings when used, GDS ports, file-storage directory, and selected slot identifiers

#### Scenario: Gateway contract remains stock F' northbound
- **WHEN** the file/downlink validation runs
- **THEN** the gateway SHALL keep stock F' framing toward `fprime-gds`
- **AND** the change SHALL NOT require a custom GDS communication plugin
