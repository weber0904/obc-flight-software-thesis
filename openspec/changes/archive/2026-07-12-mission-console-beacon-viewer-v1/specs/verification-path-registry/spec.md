## ADDED Requirements

### Requirement: Verification Path Registry Records Mission Console Beacon Viewer As A Separate Operator Path

The verification path registry SHALL record Mission Console beacon viewer proof
as a dedicated operator-facing surface distinct from lower-level UHF beacon
runtime, beacon suppress semantics, stock GDS observability, and explicit
readback families.

#### Scenario: Hosted Mission Console beacon viewer path is registered separately
- **WHEN** hosted Mission Console beacon-viewer evidence is recorded
- **THEN** the registry entry SHALL identify the path as hosted manual-surface
  beacon capability plus Mission Console dashboard/`/beacon` viewing
- **AND** it SHALL NOT over-claim stock GDS beacon visibility, RF/OTA receipt,
  or readback semantics.

#### Scenario: Target Mission Console beacon viewer path records mirrored sidecar provenance
- **WHEN** target Mission Console beacon-viewer evidence is recorded
- **THEN** the registry entry SHALL identify the path as target manual ground
  surface plus remote beacon sidecar mirror plus Mission Console viewer
- **AND** it SHALL state that the Mission Console proof consumes ground-visible
  mirrored artifacts, not a direct RF/OTA beacon receipt path.
