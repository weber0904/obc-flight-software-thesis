## MODIFIED Requirements

### Requirement: Mission Console SHALL Discover Beacon Support From Manual Surface Capability Metadata

Mission Console SHALL detect beacon availability from manual-surface manifest
capability metadata rather than inferring it from band names, stock GDS
traffic, or readback caches.

#### Scenario: Hosted and target contexts advertise beacon capability explicitly
- **WHEN** the hosted manual surface starts with beacon support or a target
  manual surface consumes ready A-published Beacon sidecar metadata
- **THEN** the surface manifest SHALL expose beacon capability metadata for the
  supported UHF operator surface
- **AND** the Mission Console gateway SHALL consume that capability to locate
  capture artifacts, frame size, and decode ancestry
- **AND** unsupported contexts SHALL remain explicit `supported=false`
  responses rather than implicit missing behavior.

#### Scenario: Target missing baseline sidecar safely degrades
- **WHEN** the target manual surface has no ready A-published Beacon sidecar
- **THEN** Mission Console SHALL not attempt a target service repair
- **AND** it SHALL expose the existing unsupported Beacon response.
