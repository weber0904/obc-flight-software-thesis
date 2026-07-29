## ADDED Requirements

### Requirement: Future Target-Bearing Dual-Link Clarification Uses Citation Guidance Without Registering A New Path

The verification-path registry SHALL add citation guidance only, and SHALL NOT
register a new simultaneous target path, when the repository freezes the future
target-bearing simultaneous dual-link claim without adding fresh proof.

#### Scenario: Clarification-only boundary does not create a new registry entry
- **WHEN** the repository records a clarification-only future target-bearing
  dual-link boundary
- **THEN** the registry SHALL keep existing simultaneous target paths
  unregistered unless fresh reviewed evidence proves one
- **AND** it SHALL not present the clarification slice itself as a newly proven
  simultaneous validation path

#### Scenario: Citation guidance binds exact reused boundaries
- **WHEN** reviewers inspect the same clarification boundary
- **THEN** the registry SHALL direct them to cite:
  - entry `59` for default target node-`5` bootstrap and primary truth
  - entry `60` for quiet or explicitly switched node-`6` command/file baseline
  - entry `60A` for quiet node-`6` suppress/runtime only
  - entry `66` for target TCP comparator use only
  - entry `67` for direct target command-path adjacency only
- **AND** it SHALL state that `target-nonquiet-background-tm-stability-v1`
  remains oracle rationale only rather than a reusable simultaneous target path

#### Scenario: Hosted layers remain non-claim support only
- **WHEN** reviewers inspect adjacent hosted simultaneous entries
- **THEN** the registry SHALL keep hosted `43B` and `43C` citations limited to
  their hosted operator and orchestration boundaries
- **AND** it SHALL NOT let those hosted entries stand in for target-bearing
  command truth
