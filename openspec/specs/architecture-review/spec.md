# architecture-review Specification

## Purpose
Define checked-in technical architecture-review packages as refreshable point-in-time review snapshots for repository baseline, subsystem completeness, verification posture, release readiness, and workflow-governance continuity from repository truth.
## Requirements
### Requirement: Checked-In Technical Architecture Review Package
The repository SHALL provide a checked-in `architecture-review` package under `docs/architecture-review/` that links to technical review snapshots for software baseline, subsystem scope, verification posture, release readiness, and workflow-governance state, and each package SHALL declare whether it is current, refreshed, stale, or a point-in-time snapshot.

#### Scenario: Reviewer finds one technical assessment entrypoint
- **WHEN** a developer or reviewer needs to inspect architecture-review material
- **THEN** they SHALL be able to find one checked-in architecture-review entrypoint that links to the subsystem dossiers, verification and release-readiness summary, and workflow-governance audit while declaring the package freshness boundary

#### Scenario: Snapshot package does not override current truth
- **WHEN** an architecture-review package is marked as a point-in-time snapshot or lacks an explicit governed refresh for the current main revision
- **THEN** developers and agents SHALL use current code, topology, scripts, archived evidence, main specs, and the verification-path registry for current baseline claims

### Requirement: Architecture Review Uses Explicit Truth Priority
The architecture-review package SHALL ground its conclusions in a documented truth-priority order of code, topology, and scripts; archived OpenSpec changes and `docs/test-records/`; current main specs; `docs/verification-path-registry.md`; and narrative, snapshot, reporting, or planning docs, and it SHALL call out any relevant drift when those sources disagree.

#### Scenario: Lower-priority docs do not override code or archived evidence
- **WHEN** a narrative, snapshot, reporting, or planning document differs from current code, archived evidence, current main specs, or the verification-path registry
- **THEN** the architecture-review package SHALL report the drift and SHALL use the higher-priority source as the current truth

### Requirement: Package Includes Fixed Deliverables
The architecture-review package SHALL include a main assessment README, a subsystem-and-capability dossier document, a verification-and-release-readiness document, a workflow-governance audit document, and one checked-in export-ready thesis summary source.

#### Scenario: Technical assessment package contains all required artifacts
- **WHEN** the architecture-review package is inspected
- **THEN** reviewers SHALL find all five checked-in artifacts needed to understand scope, proof, release posture, governance posture, and repo-external summary export

### Requirement: Subsystem Dossiers Use A Fixed Template
The subsystem-and-capability dossier document SHALL describe each covered subsystem or capability using a fixed section template that includes purpose or ownership, runtime shape, main components or helpers, interfaces and transport, public command or telemetry or event surface, simulator or support behavior, integration with other subsystems, verification status, current gaps or hardware constraints, and release-impact notes.

#### Scenario: Reviewer can compare subsystem completeness consistently
- **WHEN** a reviewer reads two subsystem sections in the dossier document
- **THEN** they SHALL be able to compare completeness and constraints without reconstructing a different template for each subsystem

### Requirement: Verification And Release Readiness Stay Explicit
The architecture-review package SHALL separate the validated hosted, Raspberry Pi local, Raspberry Pi remote, direct `OBC -> GDS`, `fprime-cli -> GDS`, external comm, and GPS fake or replay paths as of the package freshness boundary, and SHALL produce an explicit release recommendation using the categories `yes`, `yes with scope guardrails`, or `no`.

#### Scenario: Release judgement distinguishes proof from aspiration
- **WHEN** the package recommends whether a release can be cut now or could be cut at the recorded snapshot boundary
- **THEN** it SHALL state what the release proves, what it does not prove, what still blocks a hardware-integration-ready release, and which freshness boundary governs the claim

### Requirement: Workflow Governance Audit Answers Repo-Only Continuity
The architecture-review package SHALL audit whether a future agent can continue both the change workflow and the release workflow from checked-in repository context alone, and SHALL state any missing governance artifacts needed to close that gap.

#### Scenario: Audit reports change and release workflow separately
- **WHEN** the workflow-governance audit is read
- **THEN** it SHALL provide separate conclusions for repo-only change workflow continuity and repo-only release workflow continuity instead of treating them as the same problem
