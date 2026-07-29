## MODIFIED Requirements

### Requirement: Payload Capture Canonical Artifact Is An Official FDP Family
The system SHALL promote payload capture artifacts into official F' payload
`.fdp` products written through `DpManager` and `DpWriter`.

#### Scenario: Canonical payload artifacts use the official data-product path
- **WHEN** `PayloadOpsController` captures a payload image successfully
- **THEN** it SHALL publish one official payload data product for the
  auto-promoted preview JPEG
- **AND** later raw promotion SHALL also publish through the same official F'
  product ports
- **AND** `DpWriter` SHALL write those official payload artifacts under
  `<runtime-root>/data-products/`
- **AND** the payload slice SHALL NOT define a second direct file-downlink
  owner for those artifacts

### Requirement: Payload Product Family Contains Metadata And Artifact Bytes
The canonical payload `.fdp` family SHALL contain both bounded artifact
metadata and the selected artifact bytes.

#### Scenario: Payload FDP is self-describing by artifact kind
- **WHEN** the system serializes one canonical payload product
- **THEN** the product SHALL contain one artifact-oriented header record with
  capture identity, capture time, artifact kind, image dimensions,
  pixel-format/decode fields, local artifact identity, and artifact-specific
  publication state
- **AND** it SHALL contain one variable-size `U8 array` bytes record holding
  either preview JPEG bytes or raw frame bytes for that product
- **AND** the product SHALL be decodable by repo-owned payload `.fdp` tooling

### Requirement: Preview Auto-Publish And Raw On-Demand Publish Stay Distinct
The active payload `.fdp` model SHALL distinguish auto-published preview JPEG
products from later on-demand raw products.

#### Scenario: Preview and raw are not packed into one official product
- **WHEN** a payload capture succeeds
- **THEN** the controller SHALL auto-publish the preview JPEG artifact as the
  first official payload `.fdp`
- **AND** raw official publication SHALL happen only after an explicit payload
  publish command
- **AND** the active payload baseline SHALL NOT require downlinking raw bytes
  in order to receive the preview artifact

### Requirement: Payload FDP Family Ceiling Is Bounded
The active payload data-product family SHALL keep a governed default total
serialized payload-data budget of `2 MiB`.

#### Scenario: Full raw publication remains out of scope
- **WHEN** the selected official raw artifact would exceed the governed
  `2 MiB` payload family budget
- **THEN** `PayloadOpsController` SHALL reject that raw publication
- **AND** the bounded `FULL` raw official downlink path SHALL remain a non-claim
- **AND** preview JPEG products that fit the ceiling MAY still publish

### Requirement: Payload FDP Decode And Extraction Are Repository-Owned
The repository SHALL provide repo-owned tooling that decodes both legacy and
artifact-oriented payload `.fdp` families and extracts their payload bytes.

#### Scenario: Proof can validate preview and raw product contents
- **WHEN** a hosted or target proof receives a payload `.fdp` file through the
  official downlink path
- **THEN** repo-owned tooling SHALL decode the payload header fields from that
  `.fdp`
- **AND** it SHALL extract the embedded bytes into an artifact that matches the
  header-declared artifact kind
- **AND** preview-JPEG proof SHALL require valid JPEG bytes in addition to hash
  parity
- **AND** raw proof SHALL emit the raw `.bin` plus decode metadata needed to
  interpret that frame
