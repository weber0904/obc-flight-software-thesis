## ADDED Requirements

### Requirement: Checked-In Reporting Package
The repository SHALL provide a checked-in `project-reporting` package under the documentation tree that summarizes the current validated project scope, architecture, verification posture, and live-demo plan for professor or PM-facing review.

#### Scenario: Reviewer finds one reporting package entrypoint
- **WHEN** a reviewer, professor, or PM needs a current project summary
- **THEN** they SHALL be able to find one checked-in reporting package entrypoint that links to the architecture diagrams, capability matrix, and demo runbook

### Requirement: Reporting Package Includes Fixed Deliverables
The `project-reporting` package SHALL include a main briefing document, one diagram document containing the system context, internal architecture, and workflow/verification diagrams, one capability-and-status matrix, and one governed demo runbook.

#### Scenario: Reporting package contains all required artifacts
- **WHEN** the first reporting package slice is reviewed
- **THEN** the package SHALL provide the four checked-in deliverables needed for a 10-15 minute oral report plus short live demo

### Requirement: Reporting Package Uses Repository Truth
The `project-reporting` package SHALL ground its claims in checked-in repository truth, including the main specs, verification matrix, verification-path registry, reconciliation matrix, topology files, and archived evidence, instead of relying on chat-only summaries or assumed future work.

#### Scenario: Reporting summary cites governed repository truth
- **WHEN** the package describes what is already completed or validated
- **THEN** that description SHALL be consistent with the checked-in repository sources that define the current baseline

### Requirement: Diagrams Distinguish Proven And Future Scope
The reporting diagrams SHALL distinguish the currently proven hosted or Raspberry Pi paths from hardware-constrained or future architectural paths such as live GPS UART, CAN-centered expansion, dual-band integration, and scheduler/payload operations.

#### Scenario: Diagram does not over-claim future hardware work
- **WHEN** a diagram shows a future hardware or architecture direction
- **THEN** it SHALL keep that direction visually or textually distinct from the already validated baseline

### Requirement: Capability Matrix Is PM-Friendly
The reporting package SHALL provide a capability/status matrix that answers what each current capability does, which validation path has been proven, which test layers exist, what current limitation remains, and what the next step would be, without requiring the reader to navigate class-by-class implementation details.

#### Scenario: Non-domain reviewer can understand current scope
- **WHEN** a professor or PM reads the capability matrix
- **THEN** they SHALL be able to understand the completed scope and current constraints without reading source files or class names first

### Requirement: Demo Runbook Uses Stable Governed Path
The reporting package SHALL provide a demo runbook whose primary live path reuses a governed hosted baseline and whose fallback plan avoids turning the report into an unbounded hardware-integration gamble.

#### Scenario: Demo runbook prefers stable hosted baseline
- **WHEN** an operator prepares for a professor demo
- **THEN** the runbook SHALL choose a hosted, already-governed path as the primary demo and SHALL keep Raspberry Pi or future hardware paths as optional follow-up material unless separate evidence proves they are the safer choice

### Requirement: Reporting Package Teaches Boundary-Aware Communication
The reporting package SHALL explain how to present the project to a non-domain audience by separating current results, current proof, and remaining constraints instead of speaking as if the entire flight-like system were already complete.

#### Scenario: Report distinguishes result, proof, and remaining gap
- **WHEN** the reporting package teaches how to present a capability
- **THEN** it SHALL help the speaker answer what the capability does, how far it has been proven, and what remains out of scope or hardware-constrained
