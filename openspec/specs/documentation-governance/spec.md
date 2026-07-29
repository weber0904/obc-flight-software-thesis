# documentation-governance Specification

## Purpose
Define the repo-root onboarding docs, current-facing narrative-doc freshness
markers, snapshot-package scope markers, and the repo-local checker that keeps
checked-in documentation aligned with canonical repository truth.
## Requirements
### Requirement: Repo-Root Documentation Entrypoints Stay Low-Churn

The repository SHALL provide `README.md` and repo-root `AGENTS.md` as the
first onboarding entrypoints, and those files SHALL summarize the maintained
scope, source-priority guidance, current document families, and formal workflow
entrypoints without duplicating large high-churn current-baseline detail that
already lives in canonical current-truth documents.

#### Scenario: Repo-root entrypoints route readers to canonical current truth

- **WHEN** a reader starts from `README.md` or `AGENTS.md`
- **THEN** those files SHALL direct that reader to the canonical current-truth
  surfaces such as current architecture, the verification-path registry, formal
  workflow specs, roadmap notes, and evidence indexes
- **AND** they SHALL NOT act as competing detailed baseline narratives

### Requirement: Current-Facing Narrative Docs Declare Freshness

The repository SHALL require current-facing narrative docs that describe
current truth or current operator practice to carry explicit status and
freshness metadata so drift is auditable from the document itself.

#### Scenario: Current-truth doc exposes freshness boundary

- **WHEN** a reader opens a current-facing narrative doc such as the current
  architecture guide, roadmap notes, interface index, or current operator
  runbook
- **THEN** that doc SHALL identify its status
- **AND** it SHALL declare the reconciliation change, commit, or date that
  defines its freshness boundary

### Requirement: Current Non-Canonical Packages Mark Scope

The repository SHALL require current but non-canonical package entrypoints such
as thesis/reference, reporting, and refreshed architecture-review materials to
explicitly mark their freshness boundary and to state that code, topology,
specs, the verification-path registry, and evidence remain higher-priority
current truth.

#### Scenario: Snapshot-style package cannot be misread as baseline authority

- **WHEN** a reader opens a thesis, reporting, or architecture-review entrypoint
- **THEN** that entrypoint SHALL declare whether it is current, refreshed,
  stale, or a point-in-time snapshot
- **AND** it SHALL state that the package does not override higher-priority
  repository truth

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

The repository SHALL provide a repo-local documentation governance checker that
fails when required current-facing metadata is missing, when current
non-canonical package entrypoints lack explicit scope or snapshot markers, or
when current entrypoints still route readers toward retired or superseded
canonical locations.

#### Scenario: Missing current-doc metadata fails the checker

- **WHEN** a required current-facing narrative doc lacks status or freshness
  metadata
- **THEN** the documentation governance checker SHALL fail and identify the file

#### Scenario: Snapshot package boundary marker is missing

- **WHEN** a thesis, reporting, or architecture-review entrypoint lacks its
  required non-canonical or snapshot boundary wording
- **THEN** the documentation governance checker SHALL fail and identify the file

#### Scenario: Retired canonical routing fails the checker

- **WHEN** a current documentation entrypoint still routes readers to a retired
  planning surface or other superseded canonical location
- **THEN** the documentation governance checker SHALL fail and report the stale
  route

### Requirement: Manual Operator Surface Has Current Runbooks And Indexed Routing

The repository SHALL document the maintained manual dual-GDS operator family as
current operator guidance with hosted and target runbooks plus indexed routing
from current repo entrypoints.

#### Scenario: Operators can discover the manual surface without reading probe code
- **WHEN** a reader starts from `README.md`, `docs/README.md`, or
  `scripts/README.md`
- **THEN** those current entrypoints SHALL route the reader to the hosted and
  target manual dual-GDS runbooks and the `scripts/manual_ops/` subtree
- **AND** the runbooks SHALL cover stack startup, manifest reading, auth,
  secure command send, governed staged upload, `SEQ_*`, re-auth, and cleanup
  without requiring probe-code archaeology.

### Requirement: Public Documentation Has One Canonical Current Layer
The public repository SHALL use English canonical architecture, interface,
verification, contribution, and operator documents, with Traditional Chinese
limited to the repository summary, thesis claim map, and integrated demo guide.

#### Scenario: Current documentation is indexed
- **WHEN** a reviewer starts from the root or documentation README
- **THEN** each current topic SHALL route to one canonical document
- **AND** historical planning, reporting, review, and thesis-writing packages
  SHALL NOT appear as current entrypoints

### Requirement: Removed Documents Have Explicit Successors
Every high-value removed document SHALL have an exclusion reason and, where
applicable, a canonical successor in the publication manifest.

#### Scenario: Archived material names an excluded path
- **WHEN** preserved historical OpenSpec or evidence mentions an excluded
  document
- **THEN** the publication manifest SHALL explain its disposition

