## MODIFIED Requirements

### Requirement: Payload registry entries classify capture policy separately from prepared state

The verification-path registry SHALL describe payload `AUTO` and
`DETERMINISTIC` as the maintained normal capture-policy families instead of
parallel prepared-state routes.

#### Scenario: Payload registry documents persistent single-ready truth

- **GIVEN** the maintained payload target and hosted registry entries
- **WHEN** they describe the current payload still-capture path
- **THEN** they SHALL describe a single persistent normal camera-ready state
  for still capture
- **AND** they SHALL reserve special prepared-session wording only for
  `RAW_SENSOR`
- **AND** they SHALL NOT describe `PAYLOAD_CAPTURE_STILL` as a current
  maintained capture family

## ADDED Requirements

### Requirement: Payload registry records persistent-session target truth distinctly from earlier warm-up closure

The verification-path registry SHALL distinguish the current persistent-session
payload target path from the earlier target black-image warm-up closure that
still used per-capture stream start/stop semantics.

#### Scenario: Registry identifies the newer persistent-session authority

- **WHEN** reviewers inspect the maintained target real-camera payload entry
- **THEN** the registry SHALL identify the current persistent-session proof as
  the maintained authority for target non-RAW session lifecycle
- **AND** it SHALL classify the older per-capture warm-up evidence as
  historical previous closure rather than the current runtime truth
