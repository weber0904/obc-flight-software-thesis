## ADDED Requirements

### Requirement: Payload Captures Use A Governed Persistent-Data Root

The resource-storage baseline SHALL reserve a governed persistent-data path for
camera payload captures in the first payload operation slice.

#### Scenario: Payload capture files stay under persistent-data

- **WHEN** the payload contract writes a successful still image
- **THEN** the file SHALL be written under
  `<runtime-root>/persistent-data/payload/camera/`
- **AND** it SHALL NOT be written into immutable release payloads, staging,
  logs, or the official data-products runtime root

### Requirement: Payload Capture Naming Is Deterministic In V1

The first payload operation slice SHALL use a deterministic local naming rule
for camera captures.

#### Scenario: Capture file name is reviewable from runtime state

- **WHEN** a still capture completes successfully
- **THEN** the payload implementation SHALL assign the file a deterministic
  governed name such as `capture-<bootCount>-<captureCount>.jpg`
- **AND** the last-result readback SHALL allow reviewers to correlate payload
  status with that stored artifact
