## MODIFIED Requirements

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

### Requirement: Verification And Release Readiness Stay Explicit
The architecture-review package SHALL separate the validated hosted, Raspberry Pi local, Raspberry Pi remote, direct `OBC -> GDS`, `fprime-cli -> GDS`, external comm, and GPS fake or replay paths as of the package freshness boundary, and SHALL produce an explicit release recommendation using the categories `yes`, `yes with scope guardrails`, or `no`.

#### Scenario: Release judgement distinguishes proof from aspiration
- **WHEN** the package recommends whether a release can be cut now or could be cut at the recorded snapshot boundary
- **THEN** it SHALL state what the release proves, what it does not prove, what still blocks a hardware-integration-ready release, and which freshness boundary governs the claim
