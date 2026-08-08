## ADDED Requirements

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

## REMOVED Requirements

### Requirement: Checked-In Technical Architecture Review Package
**Reason**: The existing package is a stale point-in-time review and cannot be
presented as current in the thesis release.

**Migration**: Preserve its formal change provenance in OpenSpec and integrate
accepted conclusions into current architecture documents.

### Requirement: Architecture Review Uses Explicit Truth Priority
**Reason**: Truth-priority requirements now apply directly to canonical public
documents.

**Migration**: Use documentation-governance and public-release-profile.

### Requirement: Package Includes Fixed Deliverables
**Reason**: The public release does not ship a separate review package.

**Migration**: Use architecture, interfaces, verification, and evidence.

### Requirement: Subsystem Dossiers Use A Fixed Template
**Reason**: Stale dossiers are replaced by current contribution and interface
documentation.

**Migration**: Keep subsystem boundaries in canonical architecture documents.

### Requirement: Verification And Release Readiness Stay Explicit
**Reason**: Readiness belongs to the live verification matrix and release
provenance.

**Migration**: Use the public CI summary and verification documents.

### Requirement: Workflow Governance Audit Answers Repo-Only Continuity
**Reason**: Contributor and OpenSpec documentation now provide repo-only
continuity without a snapshot audit package.

**Migration**: Use `CONTRIBUTING.md` and delivery-workflow.
