## ADDED Requirements

### Requirement: PayloadOpsController Owns The Public Camera Payload Contract

The active baseline SHALL provide exactly one public payload owner for the
an OV5647-based Raspberry Pi CSI camera, and that owner SHALL be
`PayloadOpsController`.

#### Scenario: Public payload symbols stay under one owner

- **WHEN** the runtime exposes payload commands, payload status telemetry, or
  payload operation events
- **THEN** those public symbols SHALL be owned by `PayloadOpsController`
- **AND** the active baseline SHALL NOT expose raw `libcamera` or
  `rpicam-apps` command surfaces as the operator contract

### Requirement: Payload Commands Are Explicit And Sequence-Consumable

The active payload contract SHALL expose explicit prepare, capture, abort,
shutdown, status, and runtime-default configuration commands, and official
sequencing SHALL consume those commands through the normal governed command
path.

#### Scenario: Prepare is explicit

- **WHEN** an operator or official sequence requests a still capture while the
  payload is not in the prepared `READY` state
- **THEN** `PAYLOAD_CAPTURE_STILL` SHALL reject the request
- **AND** v1 SHALL NOT auto-run prepare as a hidden side effect

#### Scenario: Runtime defaults change only between prepare sessions

- **WHEN** an operator requests `PAYLOAD_SET_DEFAULTS` while the payload is not
  `OFF`
- **THEN** the command SHALL reject the request
- **AND** the next active camera session SHALL keep the configuration that was
  already prepared

#### Scenario: Official sequences consume payload commands as ordinary governed commands

- **WHEN** a validated official sequence includes `PAYLOAD_PREPARE`,
  `PAYLOAD_CAPTURE_STILL`, `PAYLOAD_ABORT`, `PAYLOAD_SHUTDOWN`, or
  `PAYLOAD_GET_STATUS`
- **THEN** the existing governed sequence path SHALL dispatch those commands
- **AND** the payload slice SHALL NOT add a parallel scheduler or second
  sequence-control plane

### Requirement: Payload Mode Gating Stays Bounded

Payload commands SHALL respect the current active spacecraft mode without
turning `PAYLOAD` mode entry itself into camera execution.

#### Scenario: Prepare and capture are PAYLOAD-mode only

- **WHEN** the current mode is not `PAYLOAD`
- **THEN** `PAYLOAD_PREPARE` and `PAYLOAD_CAPTURE_STILL` SHALL reject the
  request

#### Scenario: Cleanup and readback remain available after forced mode exit

- **WHEN** the current mode leaves `PAYLOAD` while payload cleanup or readback
  is still needed
- **THEN** `PAYLOAD_ABORT`, `PAYLOAD_SHUTDOWN`, and `PAYLOAD_GET_STATUS` SHALL
  remain callable

#### Scenario: Entering PAYLOAD mode alone has no camera side effects

- **WHEN** the mode changes into `PAYLOAD`
- **THEN** the runtime SHALL NOT power, initialize, configure, or capture from
  the camera until an explicit `PAYLOAD_*` command requests that work

### Requirement: Payload Prepare And Capture Use Deferred Outcome Truth

Payload prepare and still-capture commands SHALL report success or failure only
after the real operation result is known.

#### Scenario: Prepare failure returns command failure

- **WHEN** logical power-on, settle, backend init, or default configuration
  fails during `PAYLOAD_PREPARE`
- **THEN** the command SHALL return failure on the F Prime command path
- **AND** the controller SHALL attempt cleanup before the payload remains ready

#### Scenario: Capture failure stops later sequence work

- **WHEN** `PAYLOAD_CAPTURE_STILL` fails during an official sequence
- **THEN** the command SHALL return failure first
- **AND** later sequence commands SHALL not proceed under the normal official
  sequencer failure semantics
- **AND** the payload controller SHALL attempt cleanup after the failed command

### Requirement: Logical Payload Power Uses An EPS Proxy Channel In V1

V1 SHALL model payload power explicitly even though current lab hardware does
not use a physically switched EPS rail.

#### Scenario: Proxy power notification uses reserved channel 3

- **WHEN** payload prepare or shutdown changes logical payload power state
- **THEN** the payload implementation SHALL toggle reserved EPS simulator PDU
  channel `3` as the governed proxy notification surface

#### Scenario: EPS proxy does not claim physical rail switching

- **WHEN** the runtime or evidence describes the payload power path
- **THEN** it SHALL state that EPS channel `3` is a lab proxy notification path
- **AND** it SHALL NOT claim that the current camera hardware is physically
  disconnected from power by EPS control

### Requirement: Payload Results And Files Stay Reviewable

The payload contract SHALL provide bounded result readback and governed local
image storage.

#### Scenario: Still capture is stored under governed runtime root

- **WHEN** a still capture succeeds
- **THEN** the file SHALL be written under
  `<runtime-root>/persistent-data/payload/camera/`
- **AND** the filename SHALL follow the governed deterministic naming rule for
  this change

#### Scenario: Last result is readable without payload data products

- **WHEN** an operator requests payload status after prepare, capture, abort,
  or shutdown activity
- **THEN** the runtime SHALL provide payload state, last result code, last
  capture id, and last relative file path through bounded telemetry and/or
  status events
- **AND** v1 SHALL NOT require a payload-specific data product to review that
  result

### Requirement: Abort Is Bounded And Restorative

The payload contract SHALL define a bounded abort path that cancels active work
as far as the backend allows and restores a clean controller state.

#### Scenario: Abort during prepare or capture clears prepared state

- **WHEN** `PAYLOAD_ABORT` is issued while prepare or capture work is in
  progress
- **THEN** the controller SHALL request cancellation, release backend resources,
  and clear prepared state
- **AND** it SHALL drive the payload state back to `OFF` if cleanup succeeds or
  `FAULT` if cleanup itself fails
