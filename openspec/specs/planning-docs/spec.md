# planning-docs Specification

## Purpose
Define the repository's non-normative planning-note and roadmap-handoff space, including required freshness metadata and conflict handling against formal sources.
## Requirements
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

#### Scenario: Layered hosted orchestration wording stays explicit
- **WHEN** a roadmap or current-baseline note describes the current dual-link
  hosted situation after `comm-dual-link-orchestration-v1`
- **THEN** it SHALL distinguish:
  - the maintained per-band stock-stack baseline
  - the hosted orchestration owner above that baseline
  - deferred future target-bearing simultaneous work
- **AND** it SHALL NOT collapse those three layers into one broad
  simultaneous-dual-link claim

### Requirement: Retired Planning Redirects Do Not Carry Active Content
Retired compatibility planning paths such as `docs/planning/` SHALL NOT carry
substantive active roadmap content after the documentation reorganization.

#### Scenario: Reader follows current planning entrypoints
- **WHEN** current repository docs point readers to planning or next-work
  material
- **THEN** they SHALL point to `docs/roadmap/` rather than retired redirect
  folders or obsolete done-heavy roadmap files.

### Requirement: Closed Practical Capability Lines Leave The Active Queue Clean

Planning roadmaps SHALL allow a practical capability line that is already
closed by current specs, archived evidence, and current baseline docs to leave
the active queue while preserving exact residual non-claims in the canonical
current-truth layers.

#### Scenario: Closed practical COMM line demotes to optional broadening note
- **WHEN** the current no-RF COMM baseline already closes the practical
  capability line through current specs, archived evidence, and current
  baseline docs
- **THEN** `docs/roadmap/next-work.md` MAY remove that capability line from the
  active recommended-order queue
- **AND** it MAY replace the former active queue item with a short
  non-active optional broadening note
- **AND** it SHALL NOT continue to present optional broadening or out-of-scope
  items as active practical blockers

#### Scenario: Structural non-claims remain outside the active queue
- **WHEN** a closed practical capability line still leaves structural or
  proof-boundary non-claims
- **THEN** the roadmap SHALL rely on canonical current-baseline,
  architecture, interface, and evidence layers to keep those non-claims
  explicit
- **AND** it SHALL NOT require every retained non-claim to remain listed as an
  active future-work item

### Requirement: Public Roadmap Is Release Aligned
The public roadmap SHALL contain only current release status, residual
limitations, and future work that remains applicable at the tagged boundary.

#### Scenario: Transitional implementation work is complete
- **WHEN** accepted Mission Console or observability decisions are represented
  in code, specs, interfaces, and runbooks
- **THEN** their handoff and recommendation notes SHALL be excluded from the
  public tree

