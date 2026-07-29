## MODIFIED Requirements

### Requirement: Payload Commands Are Explicit And Sequence-Consumable
The active payload contract SHALL expose explicit prepare, capture, publish,
abort, shutdown, status, and runtime-default configuration commands, and
official sequencing SHALL consume those commands through the normal governed
command path.

#### Scenario: Capture commands accept a deterministic capture index
- **WHEN** an operator or official sequence issues any `PAYLOAD_CAPTURE_*`
  command
- **THEN** the command SHALL include a ground-chosen `captureIndex`
- **AND** that `captureIndex` SHALL determine the governed local payload
  artifact names for that capture

#### Scenario: Raw publication is explicit
- **WHEN** ground wants the raw payload artifact to enter the official stored
  and downlink baseline
- **THEN** the payload contract SHALL expose one explicit
  `PAYLOAD_PUBLISH_CAPTURE(captureIndex, artifactKind)` command
- **AND** the payload slice SHALL NOT add a general payload browse/list/select
  or download family

### Requirement: Payload Results And Files Stay Reviewable
The payload contract SHALL provide bounded result readback and governed local
artifact storage.

#### Scenario: Successful capture stores dual local artifacts
- **WHEN** a still capture succeeds
- **THEN** the runtime SHALL write one raw local artifact under
  `<runtime-root>/persistent-data/payload/camera/PIC%02X.bin`
- **AND** it SHALL write one preview JPEG local artifact under
  `<runtime-root>/persistent-data/payload/camera/PIC%02X.jpg`
- **AND** reusing the same `captureIndex` SHALL overwrite the prior local pair

#### Scenario: Final payload completion distinguishes preview and raw publication
- **WHEN** an operator requests payload status or last-capture metadata after a
  capture attempt
- **THEN** the runtime SHALL expose the capture index, capture time, local raw
  path, local preview path, raw and preview byte counts, and artifact-specific
  official `.fdp` identity plus publication state
- **AND** it SHALL allow operators to distinguish capture success, preview
  publication success/failure, and later raw publication success/failure

### Requirement: Capture Metadata Is Reviewable
The payload contract SHALL emit bounded readback metadata for successful
captures without relying on a local sidecar JSON artifact as the formal
metadata contract.

#### Scenario: Payload metadata lives in readback and official product header
- **WHEN** a payload capture succeeds
- **THEN** the runtime SHALL retain the formal capture metadata in payload
  readback surfaces and the official payload `.fdp` header
- **AND** the active baseline SHALL NOT require a local `.json` sidecar as the
  formal metadata source

### Requirement: Payload Readback Includes Canonical Product Identity
The active payload contract SHALL expose canonical payload `.fdp` identity and
publication status through existing payload readback surfaces.

#### Scenario: Last-capture metadata correlates local and canonical artifacts
- **WHEN** an operator requests payload status or last-capture metadata after a
  capture attempt
- **THEN** the runtime SHALL expose the local raw and preview paths,
  preview-`.fdp` and raw-`.fdp` relative paths, and artifact-specific
  publication results through existing payload readback surfaces
- **AND** the payload slice SHALL NOT require a new public list, select, or
  download command family to correlate those artifacts
