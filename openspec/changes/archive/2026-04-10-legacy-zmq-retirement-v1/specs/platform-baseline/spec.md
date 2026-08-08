## MODIFIED Requirements

### Requirement: Hosted Internal CSP Substrate Uses libcsp ZMQHUB Semantics
The hosted internal subsystem network SHALL use libcsp and its ZMQHUB-backed carrier for development profiles, and SHALL NOT provide project-local direct ZeroMQ request/reply EPS or ADCS business transports as an active baseline.

#### Scenario: Direct EPS/ADCS ZMQ transport is not active
- **WHEN** hosted EPS or ADCS business traffic is exercised
- **THEN** it SHALL route through libcsp node/service traffic over the governed ZMQHUB-backed substrate
- **AND** active source SHALL NOT expose direct EPS/ADCS ZMQ REQ/REP endpoint defaults or integration tests
