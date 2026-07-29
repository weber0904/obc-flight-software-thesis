## ADDED Requirements

### Requirement: Active TopCcsds Includes One Payload Owner

The active `TopCcsds` deployment SHALL include one repo-owned public payload
owner for OV5647-based Raspberry Pi CSI camera operations.

#### Scenario: Payload owner is present in the active runtime

- **WHEN** the active `TopCcsds` topology is built and configured
- **THEN** it SHALL instantiate `PayloadOpsController`
- **AND** it SHALL wire that owner into the normal command, event, telemetry,
  and runtime-support surfaces used by the active deployment

### Requirement: Camera Backend Split Is Truthful Across Hosted And Target Builds

The platform baseline SHALL describe and preserve the hosted-versus-target
camera backend split.

#### Scenario: Hosted builds use a contract-testing backend

- **WHEN** the hosted baseline is built on a development machine without target
  camera access
- **THEN** the payload implementation SHALL use a stub or fake camera backend
  that proves contract behavior without claiming real image-sensor interaction

#### Scenario: Raspberry Pi target builds use libcamera

- **WHEN** the Raspberry Pi target baseline is built for the real camera path
- **THEN** the payload implementation SHALL bind the camera backend through
  `libcamera`
- **AND** the public payload command contract SHALL remain unchanged across the
  hosted and target implementations
