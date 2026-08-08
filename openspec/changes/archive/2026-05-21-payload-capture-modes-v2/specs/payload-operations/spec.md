# payload-operations Specification Delta

## ADDED Requirements

### Requirement: Capture Modes Are Explicit

The active payload contract SHALL distinguish `AUTO` and `DETERMINISTIC`
capture sessions and SHALL keep capture-family semantics explicit in the public
payload contract.

#### Scenario: Prepare selects session kind

- **WHEN** an operator or official sequence requests payload prepare
- **THEN** the public contract SHALL identify which payload capture session kind
  is being prepared

#### Scenario: Deterministic capture uses explicit defaults and override fields

- **WHEN** a deterministic capture request omits a supported per-capture field
- **THEN** the runtime SHALL use the current deterministic default profile for
  that field

### Requirement: Capture Metadata Is Reviewable

The payload contract SHALL emit bounded readback and file-backed sidecar
metadata for successful captures.

#### Scenario: Each successful JPEG capture writes sidecar metadata

- **WHEN** a payload capture succeeds
- **THEN** the runtime SHALL write a sidecar metadata record adjacent to the
  JPEG artifact under the governed capture root
