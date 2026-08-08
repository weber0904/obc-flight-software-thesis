## MODIFIED Requirements

### Requirement: Default Hosted OBC Uses CCSDS Topology
The platform baseline SHALL make the default hosted and target-facing `OBC`
deployment the only maintained OBC deployment, and that deployment SHALL use the
active CCSDS `TopCcsds` topology while preserving currently available hosted OBC
capability surfaces except already-retired fallback surfaces.

#### Scenario: Target-facing helper defaults follow the active OBC deployment
- **WHEN** a governed source-workspace Raspberry Pi helper launches the current
  target-facing `OBC` runtime without an explicit override
- **THEN** that helper SHALL default to the active `OBC` binary
- **AND** it SHALL NOT silently default to, require, or document
  `OBC_ComFprimeLegacy` as a maintained runtime.

#### Scenario: Legacy Top is absent from maintained build registration
- **WHEN** the repository generates or builds the maintained OBC deployment
- **THEN** it SHALL NOT register `OBC_ComFprimeLegacy`, `OBC_Top`, or
  `OBC/Top` as maintained build targets
- **AND** historical evidence MAY cite those retired names only as archived
  context rather than as current build or runtime requirements.
