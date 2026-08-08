## ADDED Requirements

### Requirement: Legacy Direct-ZMQ Retirement Guardrail Is Registered
The repository verification-path registry SHALL include a guardrail entry stating that EPS/ADCS direct-ZMQ request/reply paths are retired from active source, while libcsp ZMQHUB support remains the hosted internal CSP carrier.

#### Scenario: Future changes cannot cite retired direct-ZMQ paths
- **WHEN** a later change modifies EPS or ADCS hosted internal communication
- **THEN** reviewers SHALL be able to cite the retirement entry and checker to reject active direct-ZMQ endpoint, transport, or integration-test reintroduction
