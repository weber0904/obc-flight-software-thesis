## ADDED Requirements

### Requirement: ADCS CSP Vertical Slice Evidence Is Tracked Separately
The verification evidence tree SHALL treat the hosted ADCS internal libcsp business path as distinct from the foundation-only CSP path, the direct `OBC -> GDS` ground path, the external comm path, the GPS path, and real ADCS hardware validation.

#### Scenario: ADCS CSP evidence is reviewable
- **WHEN** the ADCS CSP vertical slice passes
- **THEN** the evidence SHALL identify the hub/proxy launch, OBC/client node id, ADCS simulator node id, ADCS application service ports, exercised ADCS services, observed replies, and paths that remain unproven

#### Scenario: ADCS CSP evidence does not imply ground or hardware validation
- **WHEN** the ADCS CSP vertical slice passes
- **THEN** the evidence SHALL mark only hosted ADCS business traffic as migrated and SHALL keep ground, external comm, GPS, Raspberry Pi target, and real ADCS hardware validation explicit as out of scope or `Blocked-HW`, whichever applies
