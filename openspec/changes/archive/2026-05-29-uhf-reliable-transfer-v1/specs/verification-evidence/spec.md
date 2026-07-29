## MODIFIED Requirements

### Requirement: Reliable-transfer-v1 evidence proves both success and bounded failure

The verification evidence catalog SHALL require fresh evidence for both
success and bounded degraded behavior before `uhf-reliable-transfer-v1` is
treated as proven.

#### Scenario: Hosted proof covers the bounded switched-UHF claim

- **WHEN** hosted evidence is recorded for `uhf-reliable-transfer-v1`
- **THEN** it SHALL include one happy-path official HK `.fdp` transfer through
  the explicit-switched `uhf-primary-after-failover` node-`6` path
- **AND** it SHALL include at least one degraded ACK/no-progress case that
  triggers resend before final success
- **AND** it SHALL include at least one bounded final failure case with retry
  exhaustion and no final artifact promotion
- **AND** it SHALL state that the helper `160`-byte segment ceiling is a
  bounded helper-path fact, not a generic repo MTU claim

#### Scenario: Target/lab proof stays bounded to the quiet switched node-6 path

- **WHEN** target/lab evidence is recorded for `uhf-reliable-transfer-v1`
- **THEN** it SHALL use the current explicit-switched quiet node-`6` governed
  path
- **AND** it SHALL identify quiet mode as probe-owned diagnostic control
- **AND** it SHALL record that proof-only overrides were removed and that the
  services were restored to the normal non-quiet baseline after the proof
- **AND** it SHALL NOT restate that quiet-path proof as nominal non-quiet UHF
  reliable-transfer closure
