## MODIFIED Requirements

### Requirement: Checked-In Reporting Package
The repository SHALL provide a checked-in `project-reporting` package under the documentation tree that summarizes the validated project scope, architecture, verification posture, and live-demo plan for professor or PM-facing review as of a declared freshness boundary.

#### Scenario: Reviewer finds one reporting package entrypoint
- **WHEN** a reviewer, professor, or PM needs a project summary
- **THEN** they SHALL be able to find one checked-in reporting package entrypoint that links to the architecture diagrams, capability matrix, and demo runbook while declaring whether the package is current, refreshed, stale, or a point-in-time snapshot

### Requirement: Reporting Package Uses Repository Truth
The `project-reporting` package SHALL ground its claims in checked-in repository truth, including the main specs, verification matrix, verification-path registry, reconciliation matrix, topology files, and archived evidence available at the package freshness boundary, instead of relying on chat-only summaries or assumed future work.

#### Scenario: Reporting summary cites governed repository truth
- **WHEN** the package describes what is already completed or validated
- **THEN** that description SHALL be consistent with the checked-in repository sources that define the baseline at the declared freshness boundary

#### Scenario: Stale reporting package does not govern current work
- **WHEN** a reporting package is stale or has not been explicitly refreshed after later baseline changes
- **THEN** developers and agents SHALL use current formal specs, archived evidence, the verification-path registry, and code/topology/script state for current development roadmap and baseline claims

### Requirement: Diagrams Distinguish Proven And Future Scope
The reporting diagrams SHALL distinguish the proven hosted or Raspberry Pi paths from hardware-constrained or future architectural paths such as live GPS UART, CAN-centered expansion, dual-band integration, and scheduler/payload operations as of the package freshness boundary.

#### Scenario: Diagram does not over-claim future hardware work
- **WHEN** a diagram shows a future hardware or architecture direction
- **THEN** it SHALL keep that direction visually or textually distinct from the validated baseline recorded at the package freshness boundary

### Requirement: Capability Matrix Is PM-Friendly
The reporting package SHALL provide a capability/status matrix that answers what each package-covered capability does, which validation path had been proven at the declared freshness boundary, which test layers exist, what limitation remains at that boundary, and what the next step would be, without requiring the reader to navigate class-by-class implementation details.

#### Scenario: Non-domain reviewer can understand current scope
- **WHEN** a professor or PM reads the capability matrix
- **THEN** they SHALL be able to understand the completed scope and current constraints without reading source files or class names first
