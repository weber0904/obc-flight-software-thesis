## MODIFIED Requirements

### Requirement: Payload registry entries classify capture policy separately from prepared state

The verification-path registry SHALL describe payload `AUTO`,
`DETERMINISTIC`, and compatibility `STILL` as capture-policy families instead
of parallel prepared-state routes.

#### Scenario: Payload registry documents single-ready truth

- **GIVEN** the maintained payload target and hosted registry entries
- **WHEN** they describe the current payload still-capture path
- **THEN** they SHALL describe a single normal camera-ready state for still
  capture
- **AND** they SHALL reserve special prepared-session wording only for
  `RAW_SENSOR`
- **AND** they SHALL identify `PAYLOAD_CAPTURE_STILL` as a compatibility
  deterministic wrapper
