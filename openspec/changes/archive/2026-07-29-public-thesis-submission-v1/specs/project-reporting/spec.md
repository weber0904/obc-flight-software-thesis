## ADDED Requirements

### Requirement: Public Reporting Uses Release-Aligned Canonical Surfaces
The public repository SHALL provide reviewer-facing project scope,
architecture, verification posture, and demo routing through its README,
canonical documentation, thesis claim map, and release evidence instead of a
separate point-in-time reporting package.

#### Scenario: Professor or reviewer needs a project briefing
- **WHEN** the public repository is reviewed at the tagged boundary
- **THEN** current claims SHALL route to release-aligned canonical documents
- **AND** archived reporting changes SHALL remain available as historical
  provenance rather than current status

## REMOVED Requirements

### Requirement: Checked-In Reporting Package
**Reason**: Point-in-time professor and PM packages duplicate and lag canonical
public architecture.

**Migration**: Consolidate current project-contribution material into the
canonical architecture layer.

### Requirement: Reporting Package Includes Fixed Deliverables
**Reason**: The public release no longer ships a reporting-package family.

**Migration**: Use README, architecture, verification, and thesis claim map.

### Requirement: Reporting Package Uses Repository Truth
**Reason**: Current canonical documents now serve the public review purpose.

**Migration**: Apply source-priority rules directly to canonical documents.

### Requirement: Diagrams Distinguish Proven And Future Scope
**Reason**: Diagram requirements move to current architecture documentation.

**Migration**: Keep proven/planned distinctions in the architecture overview.

### Requirement: Capability Matrix Is PM-Friendly
**Reason**: The public verification matrix replaces the reporting snapshot.

**Migration**: Use the release-aligned verification matrix.

### Requirement: Demo Runbook Uses Stable Governed Path
**Reason**: Demo operation belongs to the maintained operator document family.

**Migration**: Use the integrated thesis demo guide and current runbooks.

### Requirement: Reporting Package Teaches Boundary-Aware Communication
**Reason**: Boundary-aware wording is required directly in README,
architecture, verification, and evidence documents.

**Migration**: Enforce the wording through documentation governance checks.
