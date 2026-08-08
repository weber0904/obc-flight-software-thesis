## ADDED Requirements

### Requirement: Reliable-transfer-v1 evidence proves both success and bounded failure

The verification evidence catalog SHALL require fresh evidence for both success
and bounded degraded behavior before `reliable-transfer-v1` is treated as
proven.

#### Scenario: Hosted proof covers the v1 reliable-transfer claim

- **WHEN** hosted evidence is recorded for `reliable-transfer-v1`
- **THEN** it SHALL include one happy-path official HK `.fdp` transfer through
  the current default S-band node-`5` path
- **AND** it SHALL include at least one degraded ACK/no-progress case that
  triggers resend before final success
- **AND** it SHALL include at least one bounded final failure case with retry
  exhaustion and no final artifact promotion

#### Scenario: Target/lab proof stays bounded to the default node-`5` path

- **WHEN** target/lab evidence is recorded for `reliable-transfer-v1`
- **THEN** it SHALL use the existing default node-`5` governed path
- **AND** it SHALL clearly state any hosted-only degraded cases that are not
  newly proven on target
- **AND** it SHALL NOT cite target TCP development-carrier evidence as the
  formal primary proof for this change
