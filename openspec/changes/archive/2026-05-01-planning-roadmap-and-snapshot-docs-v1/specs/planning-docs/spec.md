## ADDED Requirements

### Requirement: Non-Normative Planning Space
The repository SHALL provide `docs/planning/` as a non-normative space for planning notes, roadmap sketches, and multi-change handoff documents, and those documents SHALL NOT define the formal baseline, verified scope, delivery workflow, or validation-path registry.

#### Scenario: Planning note stays below formal sources
- **WHEN** a planning note describes future work or a recommended sequence
- **THEN** that note SHALL state that it is not a formal baseline and SHALL NOT override `openspec/specs/`, archived OpenSpec changes, `evidence/records/`, or `evidence/verification-path-registry.md`

### Requirement: Planning Notes Declare Freshness
Each planning note SHALL include freshness and reconciliation metadata before making roadmap claims, including `Status`, `Last reconciled against main`, `Authoritative sources checked`, `Done`, `Active`, `Next`, `Deferred`, and an explicit warning that the note is not a formal baseline.

#### Scenario: Reader sees staleness boundary first
- **WHEN** a future agent opens a planning note
- **THEN** the note SHALL identify whether it is active, stale, superseded, or historical and SHALL name the main commit and date used for its latest reconciliation

### Requirement: Formal Sources Win Conflicts
If a planning note conflicts with code, topology, scripts, archived evidence, current main specs, or the verification-path registry, the formal source SHALL govern current implementation and validation work until a later governed change updates the formal baseline.

#### Scenario: Planning note conflicts with registered evidence
- **WHEN** a planning note claims a path is proven but the verification-path registry or archived evidence does not register that path
- **THEN** the repository SHALL treat the path as unproven for current work and SHALL require a governed validation change before using it as baseline

### Requirement: Roadmaps Preserve Narrow Change Boundaries
Planning roadmaps SHALL express future work as bounded candidate changes and SHALL warn against reviving broad historical change names when the formal evidence now requires narrower proof boundaries.

#### Scenario: Broad historical change name appears in a roadmap
- **WHEN** a planning note references an older broad change name for work that now spans multiple proof boundaries
- **THEN** the note SHALL either narrow that scope or warn readers not to revive the broad name as a single catch-all governed change
