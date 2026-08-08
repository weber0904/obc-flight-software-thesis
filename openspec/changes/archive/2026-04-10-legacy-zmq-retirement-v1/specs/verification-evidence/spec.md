## ADDED Requirements

### Requirement: Legacy Direct-ZMQ Retirement Evidence Is Reviewable
The verification evidence tree SHALL record the removal of active EPS/ADCS direct-ZMQ request/reply code as a cleanup and governance result distinct from EPS/ADCS CSP path validation, ground path validation, external comm validation, GPS validation, and real hardware validation.

#### Scenario: Retirement evidence distinguishes removed code from reused CSP paths
- **WHEN** legacy direct-ZMQ retirement completes
- **THEN** the evidence SHALL cite the checker output, focused EPS/ADCS CSP smoke results, and full baseline gate
- **AND** it SHALL state that the change does not introduce new ground, external comm, GPS, or hardware coverage
