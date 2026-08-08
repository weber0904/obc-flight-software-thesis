## MODIFIED Requirements

### Requirement: Payload still capture uses a single camera-ready state

The active payload still-capture contract SHALL use a single prepared
camera-ready state for normal still capture.

#### Scenario: AUTO and DETERMINISTIC do not require separate prepared states

- **GIVEN** the payload runtime is configured and in `PAYLOAD` mode
- **AND** the payload controller has completed a successful prepare and entered
  `PSTATE_READY`
- **WHEN** the operator issues `PAYLOAD_CAPTURE_AUTO`
- **AND** later issues `PAYLOAD_CAPTURE_DETERMINISTIC`
- **THEN** both commands SHALL be admitted from the same `PSTATE_READY` window
- **AND** the controller SHALL NOT require `m_preparedSessionKind` to match
  `AUTO` or `DETERMINISTIC`

#### Scenario: RAW sensor path remains specially gated

- **GIVEN** the payload runtime is configured
- **WHEN** the operator issues a raw sensor register command or `PAYLOAD_CAPTURE_RAW`
- **THEN** the controller SHALL continue to require the `RAW_SENSOR` prepared
  path
- **AND** normal still-capture single-ready semantics SHALL NOT weaken the raw
  sensor gate

### Requirement: AUTO capture exposes actual runtime parameters

The active target payload metadata contract SHALL distinguish requested /
controller-resolved settings from actual runtime values chosen by the target
camera backend.

#### Scenario: AUTO capture returns actual runtime values

- **GIVEN** the target payload backend completes a successful `AUTO` capture
- **WHEN** the operator reads the current last-capture metadata surface
- **THEN** the readback SHALL include actual exposure, actual analogue gain,
  and actual AWB result fields
- **AND** those fields SHALL be separate from the existing
  `requestedSettings` / `appliedSettings` truth

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
