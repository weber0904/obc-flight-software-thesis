## MODIFIED Requirements

### Requirement: Hosted Runtime Refactors Preserve Operator Surface
The platform baseline SHALL preserve the hosted operator command surface and
public component contracts across hosted-runtime refactors without treating the
retired housekeeping archive file format as an active public contract.

#### Scenario: Existing operator commands remain stable
- **WHEN** a hosted runtime command-dispatch refactor is implemented
- **THEN** existing operator commands such as `status`, `mode`, `csp ping`, `eps`, `adcs`, `gps`, `storage`, `comm`, `radio`, `uart`, and `boot` SHALL remain available with their existing valid argument forms
- **AND** existing startup and status output markers used by repository-owned probes SHALL remain stable

#### Scenario: Refactor does not change public component contracts
- **WHEN** hosted runtime dispatch logic is refactored
- **THEN** the change SHALL NOT modify public F' component command opcodes, telemetry channels, event definitions, COMM CSP node IDs, COMM service ports, or COMM request/reply wire layouts

### Requirement: Default Hosted OBC Uses CCSDS Topology
The platform baseline SHALL make the default hosted `OBC` deployment the CCSDS
S-band topology while preserving the shared hosted runtime helper and all
currently available hosted OBC capability surfaces except retired HK fallback
surfaces.

#### Scenario: Default hosted OBC exposes OBCApp namespace
- **WHEN** the default hosted `OBC` deployment is built
- **THEN** its command, event, telemetry, file, dictionary, and topology namespace SHALL be `OBCApp`
- **AND** the topology SHALL import `ComCcsds.Subtopology`
- **AND** it SHALL retain EPS, ADCS, GPS, storage health, boot/update, onboard state, beacon, data-product, COMM controller, radio, UART, persistent-fault, and shared hosted runtime surfaces
- **AND** it SHALL NOT restore the retired housekeeping archive runtime or command surface as part of the active default deployment

#### Scenario: Legacy ComFprime hosted deployment remains explicit
- **WHEN** the old hosted `ComFprime` topology is needed for regression
- **THEN** it SHALL be built as `OBC_ComFprimeLegacy`
- **AND** its command, event, telemetry, file, dictionary, and topology namespace SHALL be explicitly legacy rather than `OBCApp`
- **AND** it SHALL NOT be described as the default hosted or target-facing deployment

#### Scenario: Shared runtime remains topology-adapter based
- **WHEN** the default CCSDS deployment and legacy `ComFprime` deployment expose common operator shell commands
- **THEN** common parsing, status formatting, and command dispatch SHALL remain in shared hosted runtime helper logic
- **AND** topology-specific component access SHALL remain in deployment adapters
