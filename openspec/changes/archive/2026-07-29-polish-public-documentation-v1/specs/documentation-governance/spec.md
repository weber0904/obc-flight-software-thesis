## MODIFIED Requirements

### Requirement: Repo-Root Documentation Entrypoints Stay Low-Churn

The repository SHALL provide `README.md` and `docs/README.md` as the first
reader entrypoints. They SHALL summarize the system, route readers by topic,
and avoid duplicating detailed architecture, interface, operation, or
verification material.

#### Scenario: Reviewer starts at the repository root
- **WHEN** a reviewer opens `README.md`
- **THEN** the page SHALL explain the software, principal capabilities, build,
  verification, security, and license
- **AND** it SHALL route architecture, interface, operation, specification, and
  evidence questions to their canonical files

### Requirement: Current-Facing Narrative Docs Declare Freshness

Current reader documents SHALL describe the active system directly. Git
history, the annotated tag, `release/RELEASE_PROVENANCE.md`, and evidence
records SHALL provide revision and execution provenance; reader documents
SHALL NOT require status banners, reconciliation reminders, or author-directed
release commentary.

#### Scenario: Reader opens a current document
- **WHEN** a reader opens architecture, interface, verification, operator, or
  thesis documentation
- **THEN** the document SHALL begin with its technical subject
- **AND** revision provenance SHALL remain discoverable through version control
  and linked evidence

### Requirement: Documentation Governance Checker Enforces The Rules

The repository SHALL provide a documentation checker that enforces the
canonical current-document set, required navigation, valid relative links, and
capability-first reader prose.

#### Scenario: Documentation structure drifts
- **WHEN** a reader document is missing, an overlapping document is added, a
  canonical link is broken, or release-process commentary enters the current
  layer
- **THEN** the checker SHALL fail and identify the affected file

### Requirement: Manual Operator Surface Has Current Runbooks And Indexed Routing

The repository SHALL document hosted operation, target/lab operation, and
Mission Console through one canonical guide per environment, all discoverable
from `docs/README.md`.

#### Scenario: Operator selects an environment
- **WHEN** an operator selects hosted, target/lab, or Mission Console operation
- **THEN** the applicable guide SHALL cover prerequisites, startup, status,
  authentication, representative actions, verification, troubleshooting, and
  cleanup

### Requirement: Public Documentation Has One Canonical Current Layer

The repository SHALL keep a compact `docs/` tree containing one canonical
reader document for architecture, interfaces, verification, thesis navigation,
and each operator workflow. Detailed evidence and formal governance data SHALL
live under their dedicated top-level roots.

#### Scenario: Reviewer enters the documentation tree
- **WHEN** a reviewer opens `docs/README.md`
- **THEN** every link SHALL answer a distinct reader question
- **AND** evidence records SHALL be routed through `evidence/README.md`

## REMOVED Requirements

### Requirement: Current Non-Canonical Packages Mark Scope

**Reason**: Reporting, architecture-review, and thesis-writing packages are not
part of the current reader layer.

**Migration**: Keep technical thesis navigation in `docs/thesis.md`, formal
history in OpenSpec, and result provenance in `evidence/`.
