## ADDED Requirements

### Requirement: Payload Capture Canonical Artifact Is An Official FDP
The system SHALL promote each successful payload capture into one canonical official F' payload `.fdp` artifact written through `DpManager` and `DpWriter`.

#### Scenario: Canonical payload artifact uses the official data-product path
- **WHEN** `PayloadOpsController` completes a payload capture successfully
- **THEN** it SHALL publish one canonical payload data product through official F' product ports
- **AND** `DpWriter` SHALL write the canonical payload artifact under `<runtime-root>/data-products/`
- **AND** the payload slice SHALL NOT define a second direct file-downlink owner for that artifact

### Requirement: Payload Product Contains Metadata And JPEG Bytes
The canonical payload `.fdp` SHALL contain both a bounded metadata record and the captured JPEG bytes.

#### Scenario: Payload FDP is self-describing
- **WHEN** the system serializes one canonical payload product
- **THEN** the product SHALL contain one `PayloadCaptureHeaderV1` record with capture identity, capture settings, local artifact paths, and publication state fields
- **AND** it SHALL contain one variable-size `PayloadCaptureJpegBytes` `U8 array` record holding the captured JPEG bytes
- **AND** the product SHALL be decodable by repo-owned payload `.fdp` tooling

### Requirement: Canonical Publication Is Part Of Capture Success
Payload capture command success SHALL require canonical payload `.fdp` publication in addition to local JPEG creation.

#### Scenario: Publication failure fails the capture command
- **WHEN** local payload capture produces a JPEG but canonical payload `.fdp` publication fails
- **THEN** the payload capture command SHALL return failure
- **AND** the local `.jpg + .json` artifacts MAY remain only as diagnostic residue
- **AND** readback metadata SHALL record the failed canonical publication state

### Requirement: Payload FDP Ceiling Is Bounded
The first payload data-product slice SHALL bound the canonical payload data-product size to `512 KiB`.

#### Scenario: Oversize payload capture is rejected before publish
- **WHEN** the serialized canonical payload `.fdp` data region would exceed `512 KiB`
- **THEN** `PayloadOpsController` SHALL reject canonical publication
- **AND** the capture command SHALL return failure instead of chunking the product across multiple containers
- **AND** this change SHALL NOT widen into broad arbitrary-size payload transfer governance

### Requirement: Payload FDP Decode And Extraction Are Repository-Owned
The repository SHALL provide repo-owned tooling that decodes canonical payload `.fdp` artifacts and extracts their JPEG bytes for proof oracles.

#### Scenario: Proof can validate received payload product contents
- **WHEN** a hosted or target proof receives a payload `.fdp` file through the official downlink path
- **THEN** repo-owned tooling SHALL decode the payload header fields from that `.fdp`
- **AND** it SHALL extract the embedded JPEG bytes into a reviewable artifact
- **AND** the proof SHALL be able to compare the extracted JPEG bytes against the source JPEG by hash
