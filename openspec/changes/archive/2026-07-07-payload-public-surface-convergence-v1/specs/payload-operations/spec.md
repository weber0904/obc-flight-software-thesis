## MODIFIED Requirements

### Requirement: Payload still capture uses one explicit non-RAW ready state

The active payload still-capture contract SHALL expose one explicit shared
non-RAW ready state for normal still capture and one explicit raw-only ready
state for raw register and raw-frame operations.

#### Scenario: Normal still capture uses one shared non-RAW ready state

- **GIVEN** payload camera defaults have been configured
- **AND** the payload runtime is in `PAYLOAD` mode
- **WHEN** the operator completes `PAYLOAD_PREPARE`
- **THEN** the payload SHALL enter the shared non-RAW ready state
- **AND** both `PAYLOAD_CAPTURE_AUTO` and `PAYLOAD_CAPTURE_DETERMINISTIC`
  SHALL be admitted from that same prepared window

#### Scenario: Raw capture uses an explicit raw-only prepare surface

- **WHEN** the operator needs raw register or raw-frame access
- **THEN** the payload SHALL require `PAYLOAD_PREPARE_RAW_SENSOR`
- **AND** normal non-RAW `PAYLOAD_PREPARE` SHALL NOT satisfy raw-only gates

### Requirement: Shared non-RAW session defaults are first-class

The active payload contract SHALL define shared non-RAW session-level defaults
separately from AUTO and DETERMINISTIC capture-policy defaults.

#### Scenario: Shared session defaults own resolution and preview session controls

- **WHEN** the operator configures `PAYLOAD_SET_CAMERA_DEFAULTS`
- **THEN** that surface SHALL own the current non-RAW `resolutionPreset`
- **AND** it SHALL own preview-only `jpegQuality`, `hflip`, and `vflip`

#### Scenario: AUTO defaults own only AUTO policy fields

- **WHEN** the operator configures `PAYLOAD_SET_AUTO_DEFAULTS`
- **THEN** that surface SHALL update only AUTO policy fields such as
  `awbMode`, `meteringMode`, and `evCompX100`
- **AND** it SHALL NOT redefine shared non-RAW session-level truth

#### Scenario: DETERMINISTIC defaults own only manual capture fields

- **WHEN** the operator configures `PAYLOAD_SET_DETERMINISTIC_DEFAULTS`
- **THEN** that surface SHALL update only manual deterministic capture fields
  for current maintained use
- **AND** it SHALL NOT redefine shared non-RAW session-level truth

### Requirement: Retired compatibility commands are not current payload truth

Older payload compatibility commands SHALL NOT remain on the current maintained
public payload surface after this change.

#### Scenario: Stale compatibility commands retire from the maintained surface

- **THEN** `PAYLOAD_SET_DEFAULTS` SHALL NOT remain on the maintained public
  payload command surface
- **AND** `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)` SHALL NOT remain on
  the maintained public payload command surface

### Requirement: Capture metadata distinguishes ready truth from capture policy

The active payload runtime contract SHALL not use one mixed session enum to
represent both prepare-ready truth and capture-policy truth.

#### Scenario: Status and metadata use different truth types

- **WHEN** the payload reports current prepared/active state
- **THEN** status surfaces SHALL report ready-state truth
- **AND** capture metadata, capture events, and capture manifest truth SHALL
  report capture-policy truth

### Requirement: Legacy capture manifests remain readable

The active payload contract SHALL preserve readback compatibility for captures
stored by older software revisions.

#### Scenario: Legacy manifest still loads into current readback

- **GIVEN** a capture manifest written by the older public payload metadata
  surface
- **WHEN** current software serves `PAYLOAD_GET_LAST_CAPTURE_METADATA` or
  `PAYLOAD_PUBLISH_CAPTURE` from that stored record
- **THEN** the manifest SHALL still deserialize successfully
- **AND** the runtime SHALL map the older payload truth into the current
  metadata model without corrupting current captures
