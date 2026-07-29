## ADDED Requirements

### Requirement: Checked-In Technical Architecture Review Package
The repository SHALL provide a checked-in `architecture-review` package under `docs/architecture-review/` that summarizes the current software baseline, subsystem scope, verification posture, release readiness, and workflow-governance state for technical review.

#### Scenario: Reviewer finds one technical assessment entrypoint
- **WHEN** a developer or reviewer needs to judge the current repository baseline and release posture
- **THEN** they SHALL be able to find one checked-in architecture-review entrypoint that links to the subsystem dossiers, verification and release-readiness summary, and workflow-governance audit

### Requirement: Architecture Review Uses Explicit Truth Priority
The architecture-review package SHALL ground its conclusions in a documented truth-priority order of code and topology inventory, archived changes and evidence, current main specs, narrative or reporting docs, and git history or tags, and it SHALL call out any relevant drift when those sources disagree.

#### Scenario: Lower-priority docs do not override code or archived evidence
- **WHEN** a narrative or reporting document differs from current code or archived evidence
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
The architecture-review package SHALL separate the currently validated hosted, Raspberry Pi local, Raspberry Pi remote, direct `OBC -> GDS`, `fprime-cli -> GDS`, external comm, and GPS fake or replay paths, and SHALL produce an explicit release recommendation using the categories `yes`, `yes with scope guardrails`, or `no`.

#### Scenario: Release judgement distinguishes proof from aspiration
- **WHEN** the package recommends whether a release can be cut now
- **THEN** it SHALL state what the release proves, what it does not prove, and what still blocks a hardware-integration-ready release

### Requirement: Workflow Governance Audit Answers Repo-Only Continuity
The architecture-review package SHALL audit whether a future agent can continue both the change workflow and the release workflow from checked-in repository context alone, and SHALL state any missing governance artifacts needed to close that gap.

#### Scenario: Audit reports change and release workflow separately
- **WHEN** the workflow-governance audit is read
- **THEN** it SHALL provide separate conclusions for repo-only change workflow continuity and repo-only release workflow continuity instead of treating them as the same problem
