## MODIFIED Requirements

### Requirement: Non-Normative Planning Space
The repository SHALL provide `docs/roadmap/` as the non-normative current
roadmap and handoff space for planning notes, next-work sketches, and
multi-change context. Those documents SHALL NOT define the formal baseline,
verified scope, delivery workflow, or validation-path registry.

#### Scenario: Planning note stays below formal sources
- **WHEN** a roadmap or handoff note describes future work or a recommended
  sequence
- **THEN** that note SHALL state or imply that it is not a formal baseline
- **AND** it SHALL NOT override `openspec/specs/`, archived OpenSpec changes,
  `docs/test-records/`, or `docs/verification-path-registry.md`.

### Requirement: Planning Notes Declare Freshness
Each active roadmap or handoff note SHALL include enough freshness and
reconciliation metadata to make roadmap claims auditable, including status,
last reconciliation commit or date, authoritative sources checked, current
baseline truth, active next work, deferred work, and an explicit warning that the
note is not a formal baseline when the note could be mistaken for one.

#### Scenario: Reader sees staleness boundary first
- **WHEN** a future agent opens an active roadmap or handoff note
- **THEN** the note SHALL identify whether it is active, stale, superseded, or
  historical and SHALL name the main commit or date used for its latest
  reconciliation.

## ADDED Requirements

### Requirement: Retired Planning Redirects Do Not Carry Active Content
Retired compatibility planning paths such as `docs/planning/` SHALL NOT carry
substantive active roadmap content after the documentation reorganization.

#### Scenario: Reader follows current planning entrypoints
- **WHEN** current repository docs point readers to planning or next-work
  material
- **THEN** they SHALL point to `docs/roadmap/` rather than retired redirect
  folders or obsolete done-heavy roadmap files.
