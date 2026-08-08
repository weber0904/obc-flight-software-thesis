## MODIFIED Requirements

### Requirement: Payload Commands Are Explicit And Sequence-Consumable

The active payload contract SHALL expose explicit prepare, capture, abort,
shutdown, status, and runtime-default configuration commands, and official
sequencing SHALL consume those commands through the normal governed command
path.

#### Scenario: Prepare is explicit

- **WHEN** an operator or official sequence requests a maintained normal still
  capture while the payload is not in the prepared `READY` state
- **THEN** `PAYLOAD_CAPTURE_AUTO` and `PAYLOAD_CAPTURE_DETERMINISTIC` SHALL
  reject the request
- **AND** v1 SHALL NOT auto-run prepare as a hidden side effect

#### Scenario: Runtime defaults change only between prepare sessions

- **WHEN** an operator requests `PAYLOAD_SET_DEFAULTS` while the payload is not
  `OFF`
- **THEN** the command SHALL reject the request
- **AND** the next active camera session SHALL keep the configuration that was
  already prepared

#### Scenario: Official sequences consume payload commands as ordinary governed commands

- **WHEN** a validated official sequence includes `PAYLOAD_PREPARE`,
  `PAYLOAD_CAPTURE_AUTO`, `PAYLOAD_CAPTURE_DETERMINISTIC`,
  `PAYLOAD_ABORT`, `PAYLOAD_SHUTDOWN`, or `PAYLOAD_GET_STATUS`
- **THEN** the existing governed sequence path SHALL dispatch those commands
- **AND** the payload slice SHALL NOT add a parallel scheduler or second
  sequence-control plane

#### Scenario: Capture acceptance is explicit even though final completion is deferred

- **WHEN** an accepted `PAYLOAD_CAPTURE_*` request is issued on the governed
  payload path
- **THEN** the command SHALL return success once the controller accepts the
  request and dispatches it to the payload runtime
- **AND** final capture completion SHALL be determined by payload
  state/event/tlm/readback surfaces rather than by the original command
  response

### Requirement: Payload Mode Gating Stays Bounded

Payload commands SHALL respect the current active spacecraft mode without
turning `PAYLOAD` mode entry itself into camera execution.

#### Scenario: Prepare and capture are PAYLOAD-mode only

- **WHEN** the current mode is not `PAYLOAD`
- **THEN** `PAYLOAD_PREPARE`, `PAYLOAD_CAPTURE_AUTO`, and
  `PAYLOAD_CAPTURE_DETERMINISTIC` SHALL reject the request

#### Scenario: Cleanup and readback remain available after forced mode exit

- **WHEN** the current mode leaves `PAYLOAD` while payload cleanup or readback
  is still needed
- **THEN** `PAYLOAD_ABORT`, `PAYLOAD_SHUTDOWN`, and `PAYLOAD_GET_STATUS` SHALL
  remain callable

#### Scenario: Entering PAYLOAD mode alone has no camera side effects

- **WHEN** the mode changes into `PAYLOAD`
- **THEN** the runtime SHALL NOT power, initialize, configure, or capture from
  the camera until an explicit `PAYLOAD_*` command requests that work

### Requirement: Payload still capture uses a single camera-ready state

The active payload still-capture contract SHALL use one persistent prepared
camera-ready state for normal still capture while keeping capture-policy
semantics explicit in the public payload contract.

#### Scenario: AUTO and DETERMINISTIC share one persistent non-RAW session

- **GIVEN** the payload runtime is configured and in `PAYLOAD` mode
- **AND** the payload controller has completed a successful prepare and entered
  `PSTATE_READY`
- **WHEN** the operator issues `PAYLOAD_CAPTURE_AUTO`
- **AND** later issues `PAYLOAD_CAPTURE_DETERMINISTIC`
- **THEN** both commands SHALL be admitted from the same `PSTATE_READY` window
- **AND** the controller SHALL NOT require an additional prepare merely because
  the capture policy changed

#### Scenario: RAW sensor path remains specially gated

- **GIVEN** the payload runtime is configured
- **WHEN** the operator issues a raw sensor register command or
  `PAYLOAD_CAPTURE_RAW`
- **THEN** the controller SHALL continue to require the `RAW_SENSOR` prepared
  path
- **AND** normal still-capture single-ready semantics SHALL NOT weaken the raw
  sensor gate

#### Scenario: Session-level mismatch rejects instead of silently re-preparing

- **GIVEN** the payload controller already holds an active prepared non-RAW
  session
- **WHEN** the operator requests capture that requires a different session-level
  signature such as a different resolution
- **THEN** the payload contract SHALL reject that request
- **AND** it SHALL require the operator to shut down and prepare again instead
  of silently reconfiguring during capture

### Requirement: Target Payload Backend Distinguishes Hosted Contract Proof From Real Camera Closure

The payload contract SHALL keep the public surface stable across hosted and
target backends while allowing target-only hardening behind the backend
boundary, and the current target `libcamera` still-capture path SHALL keep one
active warmed session alive across repeated maintained non-RAW captures until
shutdown, abort, or forced mode exit.

#### Scenario: Target backend hardening does not change the operator contract

- **WHEN** the real target backend changes its capture lifecycle to a persistent
  warmed session
- **THEN** the public payload command contract SHALL remain stable across hosted
  and target paths
- **AND** the change SHALL NOT require a new operator command, new session
  kind, or new FPP surface

#### Scenario: Target prepare owns stream start and warm-up

- **WHEN** the target `libcamera` backend completes `PAYLOAD_PREPARE` for a
  normal non-RAW session
- **THEN** it SHALL complete configure, allocation, stream start, and bounded
  warm-up before reporting the payload `READY`
- **AND** it SHALL keep that warmed session active for later maintained
  non-RAW captures

#### Scenario: Target capture reuses the active session

- **WHEN** the target `libcamera` backend completes multiple maintained normal
  still captures from one prepared session
- **THEN** it SHALL reuse the already-active session instead of re-running the
  full cold-start start/warm-up/stop lifecycle per capture

#### Scenario: Target capture convergence uses the configured timeout budget

- **WHEN** the target `libcamera` backend needs bounded frame convergence after
  request-level control changes
- **THEN** it SHALL use the configured `captureTimeoutMs` budget
- **AND** it SHALL NOT impose an additional fixed `3 s` capture clamp
