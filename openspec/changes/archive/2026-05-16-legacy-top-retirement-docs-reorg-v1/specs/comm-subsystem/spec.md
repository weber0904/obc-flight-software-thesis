## MODIFIED Requirements

### Requirement: Hosted S-band CCSDS Is The Default OBC Ground Path
The comm subsystem SHALL treat the default hosted `OBC` deployment as the
governed S-band CCSDS ground path. Old `ComFprime` behavior SHALL remain
reviewable only through archived historical evidence, not through a maintained
legacy OBC deployment or regression script surface.

#### Scenario: Default hosted OBC uses CCSDS S-band
- **WHEN** the default hosted `OBC` deployment is built or run
- **THEN** it SHALL use `ComCcsds` for the ground communication topology
- **AND** it SHALL expose the default command, event, telemetry, and file
  namespace as `OBCApp`
- **AND** it SHALL use S-band COMM node `5` when exercising the hosted
  gateway-backed path.

#### Scenario: Legacy ComFprime path is historical only
- **WHEN** old hosted `ComFprime` behavior is cited
- **THEN** it SHALL be described as archived historical evidence rather than as
  a maintained regression deployment
- **AND** no current script, operator guide, or verification inventory entry
  SHALL require `OBC_ComFprimeLegacy`.

#### Scenario: Existing baselines stay scoped
- **WHEN** S-band node `5` or UHF node `6` `ComFprime` baseline records are
  cited
- **THEN** they SHALL be described as stock `ComFprime` gateway baselines or
  historical records as appropriate
- **AND** they SHALL NOT be described as active CCSDS adoption evidence or as
  maintained legacy Top runtime coverage.
