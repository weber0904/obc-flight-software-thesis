## ADDED Requirements

### Requirement: Hosted Dual-Link Orchestration Owner Evidence Is Reviewable

The verification evidence tree SHALL record reviewable hosted-first evidence
for the layer-2 dual-link orchestration owner as a distinct proof boundary
above the maintained per-band stock-stack baseline.

#### Scenario: Evidence proves orchestration-owned lifecycle and failure state
- **WHEN** the repository records the hosted orchestration-owner proof
- **THEN** the evidence SHALL identify the owner manifest and status artifacts,
  lifecycle transition history, owned process set, shared-runtime interaction,
  and the final verdict
- **AND** it SHALL record a happy-path lifecycle sequence and a bounded
  startup-failure sequence owned by the orchestration surface itself

#### Scenario: Evidence avoids older COMM semantic oracles
- **WHEN** the hosted orchestration-owner evidence is finalized
- **THEN** it SHALL use orchestration-owned manifest, status, failure, and
  cleanup surfaces plus process/listener inspection as its acceptance oracle
- **AND** it SHALL NOT require `run_comm_session_and_downlink_qos_probe.sh`
  or any COMM semantic event or channel oracle to declare orchestration success

#### Scenario: Evidence keeps layer-1 and target claims separate
- **WHEN** the same orchestration-owner evidence cites adjacent paths
- **THEN** it SHALL identify `43B` as a separate layer-1 prerequisite and
  rerun it independently as non-regression
- **AND** it SHALL keep explicit non-claims for target-bearing simultaneous
  proof, one-GDS heterogeneous upstream behavior, one-gateway multiplexer
  behavior, RF closure, and any reopening of frozen UHF semantics
