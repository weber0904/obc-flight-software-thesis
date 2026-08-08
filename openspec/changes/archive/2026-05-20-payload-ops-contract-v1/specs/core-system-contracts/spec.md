## ADDED Requirements

### Requirement: Payload Commands Use The Shared Authority Model

The core system contracts capability SHALL extend the governed command
authority vocabulary to cover the public camera payload contract.

#### Scenario: Payload resource labeling is explicit

- **WHEN** the command authority catalog classifies `PAYLOAD_*` commands
- **THEN** it SHALL use a governed payload command class and payload resource
  label instead of overloading an unrelated subsystem resource

#### Scenario: Payload readback is distinguishable from mutating control

- **WHEN** the authority catalog classifies `PAYLOAD_GET_STATUS`
- **THEN** it SHALL be treated as read/status behavior
- **AND** payload mutating commands such as prepare, capture, abort, shutdown,
  and defaults update SHALL remain governed as payload-control behavior

### Requirement: Payload Mode Entry Remains Free Of Automatic Side Effects

The core mode shell contract SHALL continue to keep `PAYLOAD` mode entry
separate from real payload execution even after the first payload operation
slice lands.

#### Scenario: PAYLOAD mode does not auto-start the camera contract

- **WHEN** an operator or internal safety path enters `PAYLOAD`
- **THEN** the runtime SHALL update only the approved mode state, event, and
  telemetry contract
- **AND** it SHALL NOT implicitly invoke `PAYLOAD_PREPARE`, camera power-on,
  image capture, or payload mission execution

### Requirement: Payload Operations Reuse Existing Runtime Ownership Boundaries

The active baseline SHALL keep payload admission and execution ownership
separate from mode truth, sequence truth, and resource observability truth.

#### Scenario: Payload owner does not replace existing mode or sequence owners

- **WHEN** the payload operation path runs in the active topology
- **THEN** `PayloadOpsController` SHALL own payload admission and execution
- **AND** `ModeManager` SHALL remain the owner of current mode truth
- **AND** `SequenceAdmissionController` SHALL remain the owner of official
  sequence admission and control
- **AND** `SystemResources` SHALL remain observability only rather than a new
  payload arbitration owner
