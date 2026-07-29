## ADDED Requirements

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
