## ADDED Requirements

### Requirement: Payload persistent-session evidence distinguishes runtime lifecycle from historical single-capture warm-up evidence

The verification evidence tree SHALL record a distinct evidence package for the
current payload persistent-session lifecycle and SHALL keep it separate from the
older single-capture warm-up target proof.

#### Scenario: Persistent-session evidence records one-prepare multi-capture truth

- **WHEN** the repository records current persistent-session payload evidence
- **THEN** that evidence SHALL include the commands, observed state/readback,
  and final verdict for one `PAYLOAD_PREPARE` followed by at least
  `AUTO -> metadata -> DETERMINISTIC` on the same non-RAW session
- **AND** it SHALL state explicitly that repeated maintained captures did not
  require a second full cold-start warm-up lifecycle

#### Scenario: Persistent-session evidence records mismatch rejection

- **WHEN** the same evidence package checks non-RAW session-level mismatch
- **THEN** it SHALL record the command path and verdict showing that the runtime
  rejects the mismatch instead of silently re-preparing

### Requirement: Payload retirement evidence records current STILL removal boundary

The verification evidence tree SHALL record the retirement boundary for
`PAYLOAD_CAPTURE_STILL` so reviewers can distinguish current maintained payload
capture truth from historical compatibility evidence.

#### Scenario: Current evidence points only to AUTO and DETERMINISTIC

- **WHEN** reviewers inspect the current payload capture evidence after this
  change
- **THEN** the record SHALL identify `AUTO` and `DETERMINISTIC` as the only
  maintained normal still-capture policies
- **AND** it SHALL classify any retained `PAYLOAD_CAPTURE_STILL` records as
  historical evidence only
