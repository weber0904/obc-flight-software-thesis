## ADDED Requirements

### Requirement: Hosted ADCS Internal libcsp Path Is Registered
The repository verification-path registry SHALL include a dedicated entry for the hosted ADCS internal libcsp service path once ADCS state, mode, target, and calibration behavior have passed through ADCS node `3` on the governed hosted CSP substrate.

#### Scenario: ADCS CSP path can be reused without implying adjacent paths
- **WHEN** a later change wants to reuse hosted ADCS business traffic over libcsp
- **THEN** reviewers SHALL be able to cite one registry entry that names the ADCS internal libcsp path, its governing evidence, and the still-out-of-scope ground, external comm, GPS, Raspberry Pi target, and real ADCS hardware paths
