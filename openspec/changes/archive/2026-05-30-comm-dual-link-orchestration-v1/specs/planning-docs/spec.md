## MODIFIED Requirements

### Requirement: Roadmaps Preserve Narrow Change Boundaries

Planning roadmaps SHALL express future work as bounded candidate changes and
SHALL warn against reviving broad historical change names when the formal
evidence now requires narrower proof boundaries.

#### Scenario: Layered hosted orchestration wording stays explicit
- **WHEN** a roadmap or current-baseline note describes the current dual-link
  hosted situation after `comm-dual-link-orchestration-v1`
- **THEN** it SHALL distinguish:
  - the maintained per-band stock-stack baseline
  - the hosted orchestration owner above that baseline
  - deferred future target-bearing simultaneous work
- **AND** it SHALL NOT collapse those three layers into one broad
  simultaneous-dual-link claim
