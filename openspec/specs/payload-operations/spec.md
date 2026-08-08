# payload-operations Specification

## Purpose
Define the active baseline payload camera contract, including its public
command surface, bounded mode/power/abort semantics, and governed local
capture-result boundary.
## Requirements
### Requirement: PayloadOpsController Owns The Public Camera Payload Contract

The active baseline SHALL provide exactly one public payload owner for an
OV5647-based Raspberry Pi CSI camera, and that owner SHALL be
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

- **WHEN** an operator or official sequence requests a maintained normal still
  capture while the payload is not in the prepared `READY` state
- **THEN** `PAYLOAD_CAPTURE_AUTO` and `PAYLOAD_CAPTURE_DETERMINISTIC` SHALL
  reject the request
- **AND** v1 SHALL NOT auto-run prepare as a hidden side effect

#### Scenario: Shared and policy defaults change only while payload is off

- **WHEN** an operator requests `PAYLOAD_SET_CAMERA_DEFAULTS`,
  `PAYLOAD_SET_AUTO_DEFAULTS`, or `PAYLOAD_SET_DETERMINISTIC_DEFAULTS` while
  the payload is not `OFF`
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

### Requirement: Payload Prepare Uses Deferred Outcome Truth While Capture Completion Is State-Driven

Payload prepare SHALL report success or failure only after the real operation
result is known, while accepted still-capture commands SHALL acknowledge
dispatch immediately and surface final completion through payload state,
event, telemetry, and readback surfaces.

#### Scenario: Prepare failure returns command failure

- **WHEN** logical power-on, settle, backend init, or default configuration
  fails during `PAYLOAD_PREPARE`
- **THEN** the command SHALL return failure on the F Prime command path
- **AND** the controller SHALL attempt cleanup before the payload remains ready

#### Scenario: Capture final outcome is separated from command acknowledgement

- **WHEN** `PAYLOAD_CAPTURE_*` is accepted for execution
- **THEN** the command SHALL return success immediately
- **AND** later final success or failure, including canonical payload `.fdp`
  publication outcome, SHALL be reflected on payload state, status, telemetry,
  and metadata surfaces
- **AND** later sequence or operator logic that depends on final completion
  SHALL use those surfaces instead of the original capture command response

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

#### Scenario: Payload completion remains reviewable after async command ack

- **WHEN** an operator requests payload status after an accepted capture
  request
- **THEN** the runtime SHALL provide enough state to distinguish in-progress
  capture, post-capture publication, final success, and final failure through
  bounded telemetry and/or status events

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

### Requirement: Retired compatibility commands are not current payload truth

Older payload compatibility commands SHALL NOT remain on the current
maintained public payload surface.

#### Scenario: Historical compatibility commands stay retired

- **THEN** `PAYLOAD_SET_DEFAULTS` SHALL NOT remain on the current maintained
  payload command surface
- **AND** `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)` SHALL NOT remain on
  the current maintained payload command surface

#### Scenario: Session-level mismatch rejects instead of silently re-preparing

- **GIVEN** the payload controller already holds an active prepared non-RAW
  session
- **WHEN** the operator requests capture that requires a different session-level
  signature such as a different resolution
- **THEN** the payload contract SHALL reject that request
- **AND** it SHALL require the operator to shut down and prepare again instead
  of silently reconfiguring during capture

### Requirement: Capture Metadata Is Reviewable

The payload contract SHALL emit bounded readback and official payload-header
metadata for successful captures without depending on local `.json` sidecars.

#### Scenario: Successful capture metadata is recoverable without sidecars

- **WHEN** a payload capture succeeds
- **THEN** the runtime SHALL persist enough metadata to recover capture index,
  capture time, local artifact paths, and artifact-specific official `.fdp`
  identity for later readback or publish commands
- **AND** this change SHALL NOT require a local `.json` sidecar as part of the
  active payload contract

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

### Requirement: AUTO capture exposes actual runtime parameters

The active target payload metadata contract SHALL distinguish requested and
controller-resolved settings from actual runtime values chosen by the target
camera backend.

#### Scenario: AUTO capture returns actual runtime values

- **GIVEN** the target payload backend completes a successful `AUTO` capture
- **WHEN** the operator reads the current last-capture metadata surface
- **THEN** the readback SHALL include actual exposure, actual analogue gain,
  and actual AWB result fields
- **AND** those fields SHALL be separate from the existing requested and
  applied settings truth

### Requirement: Target deterministic support includes bounded orientation and preview quality control

The active target deterministic capture contract SHALL support manual control of
preview JPEG quality and image flip orientation in addition to exposure and
gain.

#### Scenario: Deterministic capture applies jpegQuality and flip controls

- **GIVEN** the target payload backend reports current deterministic support
- **WHEN** the operator requests deterministic capture with bounded
  `jpegQuality`, `hflip`, and `vflip`
- **THEN** the target backend SHALL either apply those fields and report them
  through current metadata
- **OR** reject them explicitly
- **AND** the active maintained baseline after this change SHALL treat them as
  supported on the current target path

### Requirement: jpegQuality affects preview JPEG only

The active payload contract SHALL define `jpegQuality` as a preview-JPEG encode
control only.

#### Scenario: jpegQuality does not affect raw artifact bytes

- **GIVEN** a payload capture that produces both raw and preview artifacts
- **WHEN** only `jpegQuality` changes
- **THEN** the raw `.bin` artifact contract SHALL remain unchanged
- **AND** only the generated preview JPEG encode path SHALL be affected

### Requirement: Raw Sensor Register Control Is Session-Bounded

The active payload contract SHALL expose raw sensor register control only while
the payload is in a prepared `RAW_SENSOR` session.

#### Scenario: Raw register control requires RAW_SENSOR session

- **WHEN** an operator requests a sensor register read or write outside a
  prepared `RAW_SENSOR` session
- **THEN** the payload contract SHALL reject the request

#### Scenario: Raw register control does not create a generic raw tunnel

- **WHEN** the payload contract exposes sensor register control
- **THEN** it SHALL expose register-oriented read/write semantics rather than an
  arbitrary byte-stream tunnel

### Requirement: Target Payload Backend Distinguishes Hosted Contract Proof From Real Camera Closure

The payload contract SHALL keep the public surface stable across hosted and
target backends while allowing target-only hardening behind the backend
boundary, and the current target `libcamera` still-capture path SHALL use
bounded warm-up sequencing before persisting a final onboard source frame.

#### Scenario: Target backend hardening does not change the operator contract

- **WHEN** the real target backend changes its capture sequencing to avoid
  cold-start black frames
- **THEN** the public payload command contract SHALL remain stable across hosted
  and target paths
- **AND** the change SHALL NOT require a new operator command, new session
  kind, or new FPP surface to access the corrected behavior

#### Scenario: Target still capture persists only a post-warm-up completed frame

- **WHEN** the target `libcamera` backend begins a still capture after
  `PAYLOAD_PREPARE`
- **THEN** it SHALL keep the selected session controls active while draining a
  bounded warm-up set of completed frames
- **AND** it SHALL persist only the first completed frame after that bounded
  warm-up window
- **AND** it SHALL fail closed rather than report capture success if that
  warm-up window cannot complete within its bounded timeout

### Requirement: Target Payload Source Artifacts Must Remain Content-Valid

The active target payload backend SHALL treat successful onboard source
artifacts as requiring bounded image-content validity rather than mere file
existence.

#### Scenario: AUTO target capture produces a non-black source artifact

- **WHEN** the target runtime completes a successful `AUTO` still capture on
  the real `OV5647` backend
- **THEN** the resulting onboard raw frame and preview JPEG SHALL be persisted
  as governed local source artifacts
- **AND** the corresponding raw source artifact SHALL satisfy the repository's
  bounded luma-validity oracle instead of matching the earlier near-black
  cold-start pattern

#### Scenario: DETERMINISTIC target capture produces a non-black source artifact

- **WHEN** the target runtime completes a successful `DETERMINISTIC` still
  capture on the real `OV5647` backend using the active explicit exposure/gain
  fields
- **THEN** the resulting onboard raw frame and preview JPEG SHALL be persisted
- **AND** the corresponding raw source artifact SHALL satisfy the same bounded
  luma-validity oracle
- **AND** this requirement SHALL NOT imply that every generic payload camera
  setting field is already implemented on the target backend

### Requirement: Payload Readback Includes Local Dual Artifacts And Official Product Identity
The active payload contract SHALL expose local raw and preview artifact identity plus artifact-specific official payload `.fdp` publication status through existing payload readback surfaces.

#### Scenario: Last-capture metadata correlates local and canonical artifacts
- **WHEN** an operator requests payload status or last-capture metadata after a capture attempt
- **THEN** the runtime SHALL expose capture index, capture time, local raw path, local preview path, preview payload `.fdp` relative path, raw payload `.fdp` relative path, and per-artifact publication result through existing payload readback surfaces
- **AND** the payload slice SHALL NOT require a new public list, select, or download command family to correlate those artifacts

### Requirement: Local Payload Files Remain Diagnostic After Canonical Promotion
The active payload contract SHALL keep local `.bin + .jpg` artifacts reviewable after official payload `.fdp` publication without describing them as the formal delivered artifact.

#### Scenario: Local payload files remain reviewable but non-canonical
- **WHEN** a payload capture succeeds and preview canonical publication also succeeds
- **THEN** the runtime SHALL preserve the governed local `.bin + .jpg` artifacts for bounded review and debugging
- **AND** active baseline wording SHALL describe the payload `.fdp` families as the formal stored/downlink artifacts instead of the local files

### Requirement: Preview Auto-Publishes While Raw Is Explicitly Promoted
The active payload contract SHALL auto-publish the preview JPEG after capture and SHALL use an explicit follow-up command to promote raw bytes.

#### Scenario: Capture command only adds capture index
- **WHEN** the public payload capture commands are used in this change
- **THEN** they SHALL retain the current preset-resolution model
- **AND** they SHALL add exactly one new ground-chosen `captureIndex` parameter for deterministic local naming

#### Scenario: Raw official publish is operator-selected
- **WHEN** an operator wants the official raw artifact for a stored capture
- **THEN** the runtime SHALL require `PAYLOAD_PUBLISH_CAPTURE(captureIndex, RAW_FRAME)`
- **AND** the runtime SHALL NOT automatically publish raw bytes as part of the original capture command
