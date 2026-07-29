# documentation-governance Specification

## Purpose
Define the repo-root onboarding docs, current-facing narrative-doc freshness
markers, snapshot-package scope markers, and the repo-local checker that keeps
checked-in documentation aligned with canonical repository truth.
## Requirements
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

### Requirement: Historical Layers Keep Current/Historical Boundary Clear

The repository SHALL keep the current/historical boundary clear even when
historical or archived document layers remain frozen, and current index or
entrypoint files SHALL repair misleading moved-path references, stale entrypoint
links, or wording that would cause historical material to be mistaken for the
current baseline.

#### Scenario: Historical index points back to the live entrypoint

- **WHEN** an archive or historical index mentions a retired path or superseded
  entrypoint
- **THEN** the checked-in current index or entrypoint SHALL either redirect the
  reader to the live canonical location or mark the historical location as
  historical only

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

### Requirement: Removed Documents Have Explicit Successors
Every high-value removed document SHALL have an exclusion reason and, where
applicable, a canonical successor in the publication manifest.

#### Scenario: Archived material names an excluded path
- **WHEN** preserved historical OpenSpec or evidence mentions an excluded
  document
- **THEN** the publication manifest SHALL explain its disposition

