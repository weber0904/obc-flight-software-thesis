## MODIFIED Requirements

### Requirement: Target Payload Backend Distinguishes Hosted Contract Proof From Real Camera Closure

The payload contract SHALL keep the public surface stable across hosted and
target backends while allowing target-only hardening behind the backend
boundary, and the current target `libcamera` still-capture path SHALL use
bounded warm-up sequencing before persisting a final onboard source frame.

#### Scenario: Target backend hardening does not change the operator contract

- **WHEN** the real target backend changes its capture sequencing to avoid
  cold-start black frames
- **THEN** the public payload command contract SHALL remain stable across
  hosted and target paths
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

## ADDED Requirements

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
