## ADDED Requirements

### Requirement: Communication Architecture Stages Are Governed Separately
The platform baseline SHALL treat software-only development, split-host development, and physical-link validation as three distinct but cumulative stages, and later stages SHALL preserve earlier stages as governed regression paths instead of replacing them.

#### Scenario: Physical-link work preserves software-only and split-host bring-up
- **WHEN** a later change introduces a physical carrier such as `CAN FD` or `UART`
- **THEN** the repository SHALL retain software-only and split-host validation paths as reusable governed baselines

### Requirement: Spacecraft-Side Internal Bus Direction Is Shared CAN FD
The near-term spacecraft-side physical carrier direction SHALL treat `EPS`, `ADCS`, and `COMM` as future CSP-facing subsystems whose traffic is intended to converge onto a shared `CAN FD` bus architecture, while direct sensor paths such as GPS remain separate where required.

#### Scenario: Shared CAN FD direction does not force GPS into the subsystem bus
- **WHEN** the repository plans future physical-carrier migration
- **THEN** it SHALL be able to move `EPS`, `ADCS`, and `COMM` toward shared `CAN FD` without requiring `GPS` to become part of that same subsystem bus baseline

### Requirement: Near-Term Hardware Role Allocation Is Reviewable
The platform baseline SHALL document the near-term host and bus role allocation used for future communication architecture work, including `obc.local` as the OBC host, `subsystem.local` as the subsystem-side host, direct OBC UART for GPS, and dual subsystem-side CAN channel groups where `EPS/ADCS` share one group and `COMM` uses the other.

#### Scenario: Future changes reuse the agreed host and bus allocation
- **WHEN** a later change begins the physical-link migration work
- **THEN** reviewers SHALL be able to cite one governed baseline statement describing the agreed OBC host, subsystem host, UART role, and subsystem-side CAN channel grouping
