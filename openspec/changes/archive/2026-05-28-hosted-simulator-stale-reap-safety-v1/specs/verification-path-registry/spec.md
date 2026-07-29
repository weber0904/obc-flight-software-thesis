## MODIFIED Requirements

### Requirement: Hosted Maintained Per-Band Stock-Stack Operator Baseline Path Is Registered

The verification-path registry SHALL include a dedicated hosted maintained
operator-baseline entry once repository-owned evidence proves one shared hosted
runtime with distinct S-band and UHF stock `fprime-gds` plus
`ground_ttc_gateway` surfaces.

#### Scenario: Registry names the shared runtime and distinct stock surfaces
- **WHEN** the hosted maintained operator-baseline proof passes
- **THEN** the registry SHALL identify one shared hosted `TopCcsds` runtime
  together with a distinct S-band stock stack and a distinct UHF stock stack
- **AND** it SHALL state that the two surfaces remain separate operator paths
  rather than one stock GDS or one gateway multiplexer path

#### Scenario: Registry cites reused prerequisites without collapsing paths
- **WHEN** reviewers inspect the hosted maintained operator-baseline entry
- **THEN** the entry SHALL cite the governing hosted operator-baseline evidence
- **AND** it SHALL identify adjacent S-band and UHF transport or policy path
  entries as reused prerequisites rather than collapsing them into one generic
  simultaneous runtime claim

#### Scenario: Registry records bounded simulator non-interference separately from orchestration claims
- **WHEN** the same entry includes hosted cleanup-hardening follow-up evidence
- **THEN** it SHALL describe that addition as bounded launcher non-interference
  with unrelated active EPS/ADCS simulator runs on different CSP hub ports
- **AND** it SHALL NOT restate that cleanup boundary as orchestration,
  simultaneous runtime arbitration, or target-bearing dual-link proof

#### Scenario: Registry keeps orchestration and target claims deferred
- **WHEN** reviewers inspect the same entry
- **THEN** it SHALL keep explicit non-claims for one-GDS heterogeneous
  upstream handling, one-gateway multiplexer behavior, simultaneous dual-link
  runtime arbitration, target-bearing simultaneous closure, and RF behavior
