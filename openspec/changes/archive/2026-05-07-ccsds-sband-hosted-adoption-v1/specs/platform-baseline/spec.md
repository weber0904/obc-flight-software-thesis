## ADDED Requirements

### Requirement: Default Hosted OBC Uses CCSDS Topology
The platform baseline SHALL make the default hosted `OBC` deployment the CCSDS S-band topology while preserving the shared hosted runtime helper and all previously available hosted OBC capability surfaces.

#### Scenario: Default hosted OBC exposes OBCApp namespace
- **WHEN** the default hosted `OBC` deployment is built
- **THEN** its command, event, telemetry, file, dictionary, and topology namespace SHALL be `OBCApp`
- **AND** the topology SHALL import `ComCcsds.Subtopology`
- **AND** it SHALL retain EPS, ADCS, GPS, storage health, boot/update, housekeeping archive, onboard state, beacon, data-product, COMM controller, radio, UART, and shared hosted runtime surfaces

#### Scenario: Legacy ComFprime hosted deployment remains explicit
- **WHEN** the old hosted `ComFprime` topology is needed
- **THEN** it SHALL be built as `OBC_ComFprimeLegacy`
- **AND** its command, event, telemetry, file, dictionary, and topology namespace SHALL be explicitly legacy rather than `OBCApp`

#### Scenario: Shared runtime remains topology-adapter based
- **WHEN** the default CCSDS deployment and legacy `ComFprime` deployment expose common operator shell commands
- **THEN** common parsing, status formatting, and command dispatch SHALL remain in shared hosted runtime helper logic
- **AND** topology-specific component access SHALL remain in deployment adapters

## REMOVED Requirements

### Requirement: Hosted Runtime Dispatch Is Shared Across Protocol Variants
**Reason**: The protocol-variant baseline changed from default `ComFprime` plus spike `ComCcsds` to default `ComCcsds` plus explicit legacy `ComFprime`.
**Migration**: Use `Default Hosted OBC Uses CCSDS Topology`.
