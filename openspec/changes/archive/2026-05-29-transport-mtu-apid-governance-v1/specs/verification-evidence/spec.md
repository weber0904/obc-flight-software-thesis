## ADDED Requirements

### Requirement: Transport governance evidence separates derivation from path scope

The verification evidence catalog SHALL require transport-governance evidence
to distinguish numeric ceiling derivation proof from reused path-scope proof
and from remaining residuals.

#### Scenario: Numeric derivation is reviewable

- **WHEN** evidence is recorded for `transport-mtu-apid-governance-v1`
- **THEN** it SHALL cite the checked-in constants, serializer sizes, and
  formulas used to derive the frozen ceiling values
- **AND** it SHALL identify the resulting current ceilings for
  `sband-primary`, `uhf-backup`, and `uhf-primary-after-failover`

#### Scenario: Reused path evidence is not overstated as numeric proof

- **WHEN** the evidence cites hosted or target/lab CCSDS records
- **THEN** it SHALL use those records to identify the operational path or APID
  flow scope that reuses the frozen contract
- **AND** it SHALL NOT claim that those reused path records directly measured
  the numeric ceiling values unless the evidence actually did so

#### Scenario: Residuals and skill-audit verdict stay explicit

- **WHEN** the transport governance evidence is finalized
- **THEN** it SHALL state which broader MTU, APID-expansion, or
  reliable-transfer questions remain intentionally unfrozen
- **AND** it SHALL state whether `.codex/skills/change-closeout/SKILL.md`
  required clarification, with a concrete reason either way

## MODIFIED Requirements

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
- **AND** it SHALL state that the `160`-byte data-segment payload ceiling is a
  bounded helper-path transport fact, not a generic repo MTU claim
