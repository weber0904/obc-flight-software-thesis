## Context

The active baseline already separates:

- mode truth in `ModeManager`
- SoC-based safety fallback and guarded mode policy in `ModeSafetyController`
- official sequence execution ownership in `SequenceAdmissionController`,
  `CmdSequencer`, and `SeqDispatcher`
- EPS command and cached-status truth in `EpsBridge`
- resource observability in `SystemResources`

`PAYLOAD` mode is currently only a shell mode. It intentionally has no camera
side effects. The camera hardware in the lab is real, but it is directly
powered from the OBC Pi camera interface rather than a real EPS-switched payload
rail. This change therefore needs to add truthful payload operation ownership
without inventing a second scheduler, a generic multi-payload platform, or a
fake physical power model.

## Goals

- add one real camera payload contract for the OV5647-based Raspberry Pi CSI
  camera
- keep `PayloadOpsController` as the only public payload owner
- keep sequencing as a consumer of payload commands, not as the payload owner
- make prepare, capture, abort, shutdown, status, and logical power sequencing
  explicit and governed
- keep `libcamera` backend details out of the OBC command contract
- prove the contract separately on hosted and Raspberry Pi target paths

## Non-Goals

- onboard scheduler or time-tagged mission planner
- persistent schedule database
- generic payload registry, plugin framework, or payload #2 abstraction
- payload autonomy, ADCS closed-loop targeting, or image downlink closure
- canonical HK snapshot, beacon, or trend-schema expansion for payload state

## Design Decisions

### 1. `PayloadOpsController` is the only public payload owner

Add one new real F' component `PayloadOpsController` under `OBC/Components/`.
It owns the public `PAYLOAD_*` command family together with payload events and
telemetry. No other component exposes public payload commands.

Its responsibilities are:

- mode-gated admission
- busy-state and state-machine admission
- deferred command completion for real operation outcome
- last-result and last-path observability
- transition to cleanup on abort or forced mode exit

This keeps the operator contract narrow and keeps command routing separate from
camera backend details.

### 2. `PiCameraManager` owns high-level camera lifecycle

`PiCameraManager` is a plain C++ support layer under `PayloadOpsController`. It
does not become a second public owner.

It owns the high-level camera lifecycle:

- logical power on/off orchestration
- power-settle delay
- backend init and release
- default parameter application
- still capture
- abort request handling

`PayloadOpsController` decides whether an operation is allowed. The manager
executes the operation.

### 3. Backend access is abstracted behind `IPiCameraDriver`

Use `IPiCameraDriver` so the public payload contract does not expose
`libcamera` details.

- `LibcameraPiCameraDriver` is built only on target-capable Linux profiles
- `StubPiCameraDriver` is used on hosted builds and tests

The target/backend split is part of the architecture truth, not an accidental
test seam. Hosted proof verifies contract behavior; Raspberry Pi target proof
verifies real camera interaction.

### 4. Payload power is modeled as a logical lifecycle with an EPS lab proxy

Current hardware truth is that the camera is directly powered from the OBC Pi
camera interface. There is no real EPS-switched payload rail. V1 therefore
models payload power as a logical lifecycle owned by `PayloadOpsController`
through `PiCameraManager`.

The manager performs two coordinated actions during logical power changes:

- notify the EPS simulator by toggling reserved proxy PDU channel `3`
- enable or release the Pi camera backend on the OBC side

The EPS toggle is a governed observability and sequencing proxy only. It is not
claimed as physical power disconnection.

### 5. Existing runtime owners remain intact

The payload slice reuses existing runtime boundaries instead of creating a new
control plane.

- `ModeManager` remains the owner of current mode truth.
- `ModeSafetyController` remains the owner of guarded safety-driven mode exits.
- `SequenceAdmissionController` remains the only official sequence owner.
- `SystemResources` remains observability only, not payload arbitration.
- `EpsBridge` remains the owner of EPS runtime status and PDU commands.

`PayloadOpsController` consumes mode truth through the same runtime-interface
style already used by `TtcPassManager`, `WatchdogSupervisor`, and
`RecoveryExecutor`. It uses a narrow payload-local EPS adapter backed by
`EpsBridge` runtime helpers for cached status and PDU control.

### 6. Public command semantics stay explicit

The public payload surface is:

- `PAYLOAD_SET_DEFAULTS(resolutionPreset, exposureUsec, gainX100)`
- `PAYLOAD_PREPARE()`
- `PAYLOAD_CAPTURE_STILL(tag, exposureOverrideUsec, gainOverrideX100)`
- `PAYLOAD_ABORT()`
- `PAYLOAD_SHUTDOWN()`
- `PAYLOAD_GET_STATUS()`

Semantics:

- `PAYLOAD_SET_DEFAULTS` updates the next prepare-session defaults only while
  the payload is `OFF`.
- `PAYLOAD_PREPARE` is explicit and required.
- `PAYLOAD_CAPTURE_STILL` runs only from the prepared `READY` state.
- `PAYLOAD_CAPTURE_STILL` does not auto-prepare.
- `PAYLOAD_PREPARE` and `PAYLOAD_CAPTURE_STILL` are allowed only while current
  mode is `PAYLOAD`.
- `PAYLOAD_ABORT`, `PAYLOAD_SHUTDOWN`, and `PAYLOAD_GET_STATUS` remain callable
  outside `PAYLOAD` so cleanup and readback still work after safety-forced
  exits.
- if the mode leaves `PAYLOAD` during prepare or capture, the controller drives
  internal abort and cleanup; it does not own the mode transition itself

### 7. Deferred completion is required for truthful sequence behavior

Prepare and capture can fail after the command handler is entered, and official
sequences need the real outcome. `PayloadOpsController` therefore keeps
deferred command completion state for the long-running operations.

The controller reports `OK`, `VALIDATION_ERROR`, or `EXECUTION_ERROR` only when
the manager has a final result. This preserves truthful behavior for direct
commands and for official sequences that contain payload commands.

If a payload command fails inside a sequence:

- the command returns failure first
- the controller then attempts cleanup
- later sequence commands do not proceed because the official sequencer sees the
  command failure

### 8. Config defaults are runtime-owned and non-persistent in v1

V1 keeps defaults in flight-software memory only. It does not add a persistent
payload profile store.

Defaults cover:

- resolution preset
- exposure time in microseconds
- gain in x100 units

Per-capture overrides are limited to:

- exposure
- gain

`JPEG` is the only v1 format. Resolution and format remain default/session-owned
to avoid per-shot pipeline churn in the first slice.

### 9. Status and result readback stay bounded

V1 uses events, telemetry, and a bounded status command rather than payload
data products.

Readback includes:

- controller state
- last result code
- logical powered/prepared truth
- last capture id
- last relative file path

`PAYLOAD_GET_STATUS()` publishes the latest status view without introducing a
parallel metadata database.

### 10. Storage stays under governed runtime roots

Still images are stored under:

`<runtime-root>/persistent-data/payload/camera/`

V1 uses deterministic filenames such as:

`capture-<bootCount>-<captureCount>.jpg`

This change governs local save behavior only. It does not claim image download
policy, downlink closure, or final retention policy.

### 11. Abort semantics are bounded but real

`PAYLOAD_ABORT` means:

- mark the current prepare or capture operation as cancelled
- request the manager/driver to stop work as far as the backend allows
- release backend resources
- clear prepared state
- drive the controller back to `OFF` if cleanup succeeds or `FAULT` if cleanup
  itself fails

The target `libcamera` backend only needs bounded v1 semantics. It does not
need to guarantee arbitrary low-latency preemption beyond the documented
prepare/capture path.

## State Model

The controller keeps a narrow state machine:

- `OFF`
- `PREPARING`
- `READY`
- `CAPTURING`
- `ABORTING`
- `FAULT`

Valid flows:

- `OFF -> PREPARING -> READY`
- `READY -> CAPTURING -> READY`
- `READY -> OFF` through shutdown
- `PREPARING` or `CAPTURING -> ABORTING -> OFF|FAULT`
- forced mode exit during `READY` may drive cleanup directly to `OFF|FAULT`

## Verification Strategy

- classic F' L2 component harness for `PayloadOpsController`
- direct helper tests for manager/driver-facing logic that owns nontrivial
  behavior
- hosted probe with the stub driver to prove:
  - `MODE_SET(PAYLOAD)` plus
    `PAYLOAD_PREPARE -> PAYLOAD_CAPTURE_STILL -> PAYLOAD_SHUTDOWN`
  - sequence use of payload commands through official sequencing
  - EPS proxy channel `3` behavior
  - bounded status and file-path readback
- target Raspberry Pi probe with `libcamera` to prove:
  - real camera init and still capture
  - EPS proxy channel `3` notification path
  - governed storage path and cleanup behavior

Hosted proof is contract proof only. Closeout-ready status still requires the
target Pi camera proof.

## Risks And Trade-offs

- [Risk] Payload controller needs long-running command behavior while still
  fitting existing F' command expectations. -> Mitigation: use explicit
  deferred completion and a narrow controller state machine.
- [Risk] Current lab hardware cannot prove real EPS rail switching. ->
  Mitigation: document the lab proxy boundary clearly and prove the EPS proxy
  toggle separately from the real camera backend behavior.
- [Risk] `libcamera` is target-only and may not build on hosted macOS. ->
  Mitigation: keep the driver split explicit and verify the operator contract
  with a hosted stub backend.
