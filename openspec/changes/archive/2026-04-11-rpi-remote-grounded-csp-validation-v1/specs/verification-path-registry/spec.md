## ADDED Requirements

### Requirement: Remote Pi-To-macOS Internal CSP Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the remote topology where the Raspberry Pi target OBC uses a macOS-hosted CSP hub plus remote EPS node `2` and ADCS node `3`.

#### Scenario: Later changes can cite the remote internal CSP topology
- **WHEN** a later change needs to reuse the governed `Pi OBC -> remote macOS simulators` baseline
- **THEN** reviewers SHALL be able to cite one registry entry that names the remote internal CSP path, its governing evidence, and the still-out-of-scope neighboring paths

### Requirement: Remote Target-Side GDS Subsystem Command Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the target-side `fprime-cli -> GDS -> Pi OBC -> remote EPS/ADCS simulator` subsystem command path once the governed probe proves one EPS and one ADCS command through that route.

#### Scenario: Ground-driven subsystem commands stay distinct from adjacent paths
- **WHEN** a reviewer checks whether the remote target-side subsystem command path is already proven
- **THEN** the registry SHALL show that this path covers only the bounded EPS and ADCS command flow through GDS and SHALL keep direct adapter connectivity, external comm, GPS, and future physical-bus behavior distinct
