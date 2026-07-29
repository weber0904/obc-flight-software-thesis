## ADDED Requirements

### Requirement: Three-Host Split-Host Internal CSP Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the three-host topology where `obc.local` reaches remote EPS node `2` and ADCS node `3` on `subsystem.local` through a macOS-hosted CSP hub.

#### Scenario: Reviewers can distinguish two-host and three-host remote CSP paths
- **WHEN** a later change needs to reuse the split-host subsystem baseline
- **THEN** reviewers SHALL be able to cite a registry entry that identifies `subsystem.local` as a separate simulator host instead of reusing the older two-host `Pi OBC -> remote macOS simulators` path

### Requirement: Three-Host Target-Side GDS Subsystem Command Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the bounded `macOS fprime-cli -> GDS -> obc.local -> subsystem.local` subsystem command path once the governed probe proves one EPS and one ADCS command through that route.

#### Scenario: Three-host ground-driven subsystem commands stay distinct
- **WHEN** a reviewer checks whether the split-host subsystem command path is already proven
- **THEN** the registry SHALL show that this path covers only the bounded EPS and ADCS command flow through GDS and the split-host CSP topology, while keeping direct adapter connectivity, external comm, GPS, and future physical-bus behavior distinct

### Requirement: Three-Host CSP Plus External Comm Coexistence Path Is Registered
The repository verification-path registry SHALL include a distinct entry for the three-host validation path where `obc.local` runs `/dev/serial0` external comm while `subsystem.local` still provides remote EPS and ADCS simulator nodes through the macOS-hosted CSP hub.

#### Scenario: Coexistence path can be cited without over-claiming adjacent behavior
- **WHEN** a later target-side change needs to rely on split-host subsystem reachability plus baseline external comm coexistence
- **THEN** reviewers SHALL be able to cite one registry entry that names that coexistence path and its still-out-of-scope neighboring paths
