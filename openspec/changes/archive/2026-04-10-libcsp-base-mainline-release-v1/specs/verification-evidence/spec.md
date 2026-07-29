## ADDED Requirements

### Requirement: Libcsp Mainline Release Evidence
The verification evidence tree SHALL record the local verification commands and result summary used to qualify the libcsp integration base for merge back to `main`.

#### Scenario: Release readiness evidence is reviewable
- **WHEN** the libcsp integration base is ready for a mainline PR
- **THEN** the evidence SHALL identify the branch, the included archived CSP migration changes, the legacy-ZMQ checker result, OpenSpec validation result, and shared baseline gate verdict

#### Scenario: Release evidence does not over-claim hardware coverage
- **WHEN** the release-readiness evidence passes
- **THEN** it SHALL still leave Raspberry Pi CSP execution, external comm UART hardware, GPS live UART, and real EPS/ADCS hardware as separately governed paths
