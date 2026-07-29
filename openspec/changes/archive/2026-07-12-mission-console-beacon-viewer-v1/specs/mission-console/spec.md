## ADDED Requirements

### Requirement: Mission Console SHALL Provide A Dedicated Beacon Viewer Surface

Mission Console SHALL provide a dedicated beacon viewer surface for hosted and
target manual operator contexts without reclassifying beacon as stock GDS
event/channel traffic or explicit readback.

#### Scenario: Dashboard shows only the latest beacon summary
- **WHEN** a selected Mission Console context exposes a supported beacon
  capability
- **THEN** the dashboard SHALL show only the latest beacon time and sequence in
  its Beacon card
- **AND** it SHALL keep detailed provenance, decode status, and field payload
  off the dashboard summary surface
- **AND** unavailable beacon state SHALL remain a bounded summary condition,
  not a raw diagnostics dump.

#### Scenario: Beacon page shows decode detail and provenance
- **WHEN** an operator opens the Mission Console `/beacon` page for a context
  with beacon capability
- **THEN** Mission Console SHALL show latest decoded beacon detail, bounded
  beacon history, source kind, source band, capture-path metadata, and decode
  status
- **AND** it SHALL distinguish hosted local PTY capture from target remote
  sidecar capture
- **AND** it SHALL NOT present that surface as a stock GDS readback or RF/OTA
  receipt claim.

### Requirement: Mission Console SHALL Discover Beacon Support From Manual Surface Capability Metadata

Mission Console SHALL detect beacon availability from manual-surface manifest
capability metadata rather than inferring it from band names, stock GDS
traffic, or readback caches.

#### Scenario: Hosted and target contexts advertise beacon capability explicitly
- **WHEN** the hosted or target manual surface starts with beacon support
- **THEN** the surface manifest SHALL expose beacon capability metadata for the
  supported UHF operator surface
- **AND** the Mission Console gateway SHALL consume that capability to locate
  capture artifacts, frame size, and decode ancestry
- **AND** unsupported contexts SHALL remain explicit `supported=false`
  responses rather than implicit missing behavior.
