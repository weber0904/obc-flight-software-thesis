## MODIFIED Requirements

### Requirement: Bounded First-Version NMEA Parsing
The first GPS slice SHALL parse a bounded first-version NMEA subset sufficient to derive fix validity, position, time, and basic reception metadata, it SHALL reject malformed or checksum-invalid sentences instead of treating them as valid fixes, and it SHALL enforce explicit first-version sentence-length and field-count bounds before continuing per-field parsing work.

#### Scenario: Valid sentence updates cached GPS state
- **WHEN** the GPS bridge receives a valid supported NMEA sentence from the current source
- **THEN** it SHALL update the cached GPS state with the fields owned by that sentence

#### Scenario: Invalid sentence preserves last valid fix
- **WHEN** the GPS bridge receives a malformed or checksum-invalid supported sentence
- **THEN** it SHALL preserve the last valid cached fix and SHALL raise the owned degraded or parse-fault path

#### Scenario: Oversized sentence is rejected as malformed
- **WHEN** the GPS bridge receives a supported sentence whose total length or field count exceeds the first-version parser bounds
- **THEN** it SHALL reject that sentence as `MALFORMED` instead of attempting unbounded parse work
