# architecture-review Specification

## Purpose
Define checked-in technical architecture-review packages as refreshable point-in-time review snapshots for repository baseline, subsystem completeness, verification posture, release readiness, and workflow-governance continuity from repository truth.
## Requirements
### Requirement: Public Architecture Review Uses Canonical Current Documents
The public repository SHALL preserve architecture-review decisions through
archived OpenSpec while routing current review through canonical architecture,
interface, verification, and evidence documents instead of a point-in-time
review package.

#### Scenario: Reviewer needs the current architecture assessment
- **WHEN** a reviewer follows the public documentation index
- **THEN** the reviewer SHALL reach current canonical documents
- **AND** archived architecture-review changes SHALL remain available as
  historical decision provenance
