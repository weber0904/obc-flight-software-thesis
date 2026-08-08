## ADDED Requirements

### Requirement: Target Direct-Control Matrix Evidence Stays Separate

Target direct-control matrix evidence SHALL remain separate from satcom and
southbound parity claims.

#### Scenario: Target direct-control proof is reviewable
- **WHEN** a target TCP or target CAN direct-control cell passes
- **THEN** the evidence SHALL show the target direct `OBC -> GDS` path with
  isolated artifacts and without routing through `ground_ttc_gateway`, node `5`,
  or node `6`

### Requirement: Target Direct-Control Wrappers Are Rerun-Safe

Dedicated target direct-control wrappers SHALL not recreate the earlier
repo-root sequence alias residue or similar owned artifacts outside their case
roots.

#### Scenario: Target direct-control rerun leaves no repo-root residue
- **WHEN** a target direct-control wrapper is interrupted or rerun
- **THEN** the wrapper SHALL preserve isolated case-owned roots and SHALL NOT
  leave repo-root staging or sequence alias artifacts behind
