## MODIFIED Requirements

### Requirement: Hosted Per-Band Stock Ground Stacks Are Maintained

The ground TT&C gateway capability SHALL continue to provide maintained
hosted-first launcher surfaces for the current near-term simultaneous
multi-band operator baseline using one shared hosted runtime plus distinct
stock S-band and UHF ground stacks.

#### Scenario: Combined wrapper remains composition-only
- **WHEN** an operator starts the maintained combined hosted wrapper
- **THEN** it SHALL continue to compose one shared hosted runtime with the
  maintained S-band and UHF stock stacks
- **AND** it SHALL report startup order and coordinated shutdown or cleanup for
  the owned processes
- **AND** it SHALL NOT claim that the wrapper is a new runtime policy owner,
  multiplexer, or higher-level orchestration surface

## ADDED Requirements

### Requirement: Hosted Dual-Link Orchestration Owner Is Distinct From The Maintained Baseline

The ground TT&C gateway capability SHALL allow a hosted-only layer-2 dual-link
orchestration owner above the maintained per-band stock-stack baseline, and
that owner SHALL remain distinct from both the layer-1 composition-only wrapper
and the underlying per-band stock surfaces.

#### Scenario: Orchestration owner declares lifecycle ownership only
- **WHEN** the hosted orchestration owner starts
- **THEN** it SHALL declare itself as a hosted-only lifecycle owner for the
  combined operator surface
- **AND** it SHALL own lifecycle, startup-failure, and cleanup-summary state
- **AND** it SHALL NOT claim command authority ownership, gateway relay
  ownership, stock-GDS plugin behavior, COMM runtime ownership, or reliable
  transfer ownership

#### Scenario: Orchestration owner records delegated adjacent state explicitly
- **WHEN** reviewers inspect the hosted orchestration owner artifact
- **THEN** they SHALL see the maintained per-band stock-stack baseline recorded
  as a delegated prerequisite
- **AND** they SHALL see per-band TT&C semantics, gateway relay behavior,
  stock GDS behavior, and COMM runtime policy listed as non-owned adjacent
  state rather than as orchestration-owned state

### Requirement: Hosted Dual-Link Orchestration Runbook Is Reviewable

The ground TT&C gateway capability SHALL include a dedicated hosted runbook for
the layer-2 dual-link orchestration owner.

#### Scenario: Runbook records the three-layer boundary
- **WHEN** an operator or reviewer follows the hosted orchestration runbook
- **THEN** it SHALL distinguish:
  - the layer-1 maintained per-band stock-stack baseline
  - the layer-2 hosted orchestration owner
  - deferred future target-bearing simultaneous work
- **AND** it SHALL state that the orchestration surface is not a gateway
  multiplexer, not a stock-GDS plugin, and not a target-bearing proof surface
