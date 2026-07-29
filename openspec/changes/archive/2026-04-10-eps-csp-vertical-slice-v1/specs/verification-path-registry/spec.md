## ADDED Requirements

### Requirement: Hosted EPS Internal libcsp Path Is Registered
The verification-path registry SHALL include a hosted EPS internal libcsp path once `EPS_GET_STATUS`, `EPS_SET_PDU`, heater/config, and reset behavior have passed through EPS node `2` on the governed hosted CSP substrate.

#### Scenario: EPS path is reused by later changes
- **WHEN** a later change relies on hosted EPS business traffic over libcsp
- **THEN** that change SHALL cite the hosted EPS internal libcsp registry entry instead of citing only the foundation-level CSP path
