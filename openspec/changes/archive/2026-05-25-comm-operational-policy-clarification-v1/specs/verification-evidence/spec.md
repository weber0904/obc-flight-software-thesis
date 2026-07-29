## ADDED Requirements

### Requirement: Policy Clarification Changes Reuse Evidence And Preserve Non-Claims

A policy-clarification change that does not add new transport proof SHALL reuse
existing governed evidence records for any claimed current behavior and SHALL
preserve unproven behavior as non-claims, bounded assumptions, or explicit
future follow-up.

#### Scenario: COMM policy clarification stays evidence-backed without inventing new proof
- **WHEN** a clarification change cites current `SESSION_OPEN` lifecycle,
  target node-`5` or node-`6` behavior, radio or link observability, beacon
  capture, or current file/downlink role boundaries
- **THEN** it SHALL cite the existing governing evidence records rather than a
  chat summary
- **AND** it SHALL NOT claim new reliable-transfer, dual-link runtime, or
  session-aware beacon suppress closure unless fresh evidence is added

## MODIFIED Requirements

### Requirement: Target Node-6 Quiet UHF Evidence Is Reviewable
The verification evidence tree SHALL record reviewable target/lab evidence for
node `6` under both `uhf-primary-after-failover` and `uhf-backup` bounded
quiet-mode proofs.

#### Scenario: Evidence records both node-6 role variants
- **WHEN** target node-`6` migration evidence is recorded
- **THEN** the evidence SHALL identify separate bounded results for
  `uhf-primary-after-failover` and `uhf-backup`
- **AND** it SHALL record that quiet mode was probe-owned and not a new nominal
  operator baseline
- **AND** it SHALL record that operator profile `uhf-primary` evidence used an
  explicit node-`5` switch-to-UHF step before node-`6` command/readback and
  therefore exercised the `uhf-primary-after-failover` runtime role
