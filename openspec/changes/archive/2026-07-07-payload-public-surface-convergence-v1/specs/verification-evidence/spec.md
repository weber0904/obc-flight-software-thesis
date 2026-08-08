## MODIFIED Requirements

### Requirement: Current payload records distinguish converged and historical surfaces

Current maintained payload evidence SHALL separate the converged public payload
surface from superseded compatibility commands and naming.

#### Scenario: Current record classifies retired compatibility commands as historical

- **WHEN** a current payload record references older payload command ancestry
- **THEN** it SHALL mark `PAYLOAD_SET_DEFAULTS` and
  `PAYLOAD_PREPARE_SESSION(AUTO|DETERMINISTIC)` as historical or superseded
- **AND** it SHALL avoid citing them as current maintained operator truth

#### Scenario: Current record distinguishes ready truth from capture-policy truth

- **WHEN** a current payload record documents runtime readback or source
  artifacts
- **THEN** it SHALL describe shared non-RAW ready truth separately from
  capture-policy truth
- **AND** it SHALL not present old `PSESSION_*` wording as the current public
  payload model
