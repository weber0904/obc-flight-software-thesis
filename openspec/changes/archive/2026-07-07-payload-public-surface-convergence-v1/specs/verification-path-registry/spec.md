## MODIFIED Requirements

### Requirement: Payload registry points to the converged public surface

The verification-path registry SHALL point current maintained payload routes to
the converged command and metadata surface introduced by this change.

#### Scenario: Current payload route does not cite retired compatibility commands

- **WHEN** the registry describes a current maintained payload proof or Route 1
  payload wrapper
- **THEN** it SHALL cite `PAYLOAD_SET_CAMERA_DEFAULTS`, `PAYLOAD_PREPARE`,
  `PAYLOAD_CAPTURE_AUTO`, `PAYLOAD_CAPTURE_DETERMINISTIC`, and
  `PAYLOAD_PREPARE_RAW_SENSOR` where relevant
- **AND** it SHALL not cite `PAYLOAD_SET_DEFAULTS` or
  `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)` as current maintained truth

#### Scenario: Current payload registry describes current naming

- **WHEN** the registry summarizes payload metadata or capture state truth
- **THEN** it SHALL use the converged ready-state and capture-policy naming
- **AND** it SHALL classify old `PSESSION_*` wording as historical naming only
