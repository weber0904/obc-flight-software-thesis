# payload-data-products Specification

## Purpose
Define the canonical payload data-product family, its bounded publication
rules, and the repo-owned decode oracles that support hosted and target proof.

## Requirements

### Requirement: Payload Capture Canonical Artifact Is An Official FDP Family

The system SHALL promote each successful payload capture into one canonical
official F' payload `.fdp` family written through `DpManager` and `DpWriter`.

#### Scenario: Canonical payload artifact uses the official data-product path

- **WHEN** `PayloadOpsController` completes a payload capture successfully
- **THEN** it SHALL publish one canonical payload data-product family through
  official F' product ports
- **AND** `DpWriter` SHALL write the canonical payload artifact family under
  `<runtime-root>/data-products/`
- **AND** the payload slice SHALL NOT define a second direct file-downlink
  owner for that artifact family

### Requirement: Payload Product Family Contains Metadata And One Explicit Artifact Kind

The canonical payload `.fdp` family SHALL contain both a bounded metadata
record and exactly one explicit payload artifact kind per publication.

#### Scenario: Payload FDP family is self-describing

- **WHEN** the system serializes one canonical payload product family
- **THEN** each published family member SHALL contain one
  `PayloadCaptureArtifactHeaderV2` record with capture identity, capture
  index, capture time, resolution, important applied settings, local raw and
  preview paths, and artifact-specific official `.fdp` path fields
- **AND** the family SHALL carry one explicit `artifactKind`
- **AND** the family SHALL contain one or more variable-size
  `PayloadCaptureArtifactBytes` `U8` array records whose concatenation equals
  the selected artifact bytes
- **AND** `PREVIEW_JPEG` and `RAW_FRAME` SHALL be distinct official payload
  artifact kinds rather than two payload artifacts packed into one `.fdp`
  family
- **AND** the product family SHALL be decodable by repo-owned payload `.fdp`
  tooling

### Requirement: Final Preview Success Requires Canonical Publication

Final payload preview success SHALL require canonical preview payload `.fdp`
family publication in addition to local raw plus preview creation.

#### Scenario: Preview publication failure becomes deferred payload failure after async ack

- **WHEN** local payload capture produces a raw artifact and preview JPEG but canonical preview payload `.fdp`
  family publication fails
- **THEN** the accepted capture command MAY already have returned success for
  dispatch acknowledgement
- **AND** the payload state, telemetry, status, and metadata surfaces SHALL
  report final failure
- **AND** the local `.bin + .jpg` artifacts MAY remain only as diagnostic
  residue

### Requirement: Payload FDP Family Ceiling Is Bounded

The active payload data-product family SHALL use a governed default total
serialized payload-data budget of `2 MiB` while allowing multi-slice family
publication within that bound.

#### Scenario: Oversize payload capture is rejected above the governed family budget

- **WHEN** the serialized canonical payload `.fdp` family data region would
  exceed `2 MiB`
- **THEN** `PayloadOpsController` SHALL reject canonical publication
- **AND** the payload completion surfaces SHALL report final failure instead of
  claiming successful canonical delivery
- **AND** payload captures that fit within that bound MAY span multiple
  official `.fdp` family members

### Requirement: Preview Auto-Publishes While Raw Promotes On Demand

The active payload baseline SHALL auto-publish the preview JPEG artifact after
capture and SHALL require an explicit follow-up command before raw bytes are
promoted into an official payload `.fdp` family.

#### Scenario: Raw official publish is separately selected

- **WHEN** a successful capture stores both local raw and local preview
  artifacts
- **THEN** the controller SHALL auto-publish only `PREVIEW_JPEG`
- **AND** `RAW_FRAME` SHALL be promoted only by a later explicit payload
  publish command that names the `captureIndex`
- **AND** the change SHALL NOT add a payload-specific browse/list/select/
  download command family beyond the official `BUILD_CATALOG` plus
  `START_XMIT_CATALOG` path

### Requirement: Full Raw Official Publish Stays Deferred

The active payload baseline SHALL keep `FULL` raw official publish outside the
current governed claim while retaining the current `2 MiB` payload family
ceiling.

#### Scenario: Full raw publish rejects with bounded deferred detail

- **WHEN** a stored `FULL` raw capture is promoted into an official
  `RAW_FRAME` payload `.fdp` family
- **THEN** the controller SHALL reject that promotion with an explicit bounded
  deferred result/detail
- **AND** this change SHALL still allow bounded `VGA` and `HD` raw official
  publication within the current family model

### Requirement: Payload FDP Decode And Extraction Are Repository-Owned

The repository SHALL provide repo-owned tooling that decodes canonical payload
`.fdp` families and extracts the selected payload artifact for proof oracles.

#### Scenario: Proof can validate received payload family contents

- **WHEN** a hosted or target proof receives a payload `.fdp` family through
  the official downlink path
- **THEN** repo-owned tooling SHALL decode the payload header fields from that
  family
- **AND** it SHALL extract the embedded `PREVIEW_JPEG` bytes into a reviewable
  `.jpg` artifact or the embedded `RAW_FRAME` bytes into a reviewable `.bin`
  artifact
- **AND** preview proof SHALL require a valid JPEG payload
- **AND** raw proof SHALL be able to compare the extracted raw bytes against
  the source raw artifact by hash
