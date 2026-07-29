## ADDED Requirements

### Requirement: Comm Is The Future Ground-Facing Spacecraft Subsystem
The comm subsystem SHALL be treated as the long-term spacecraft-side subsystem responsible for handling omitted-RF ground TT&C traffic before that traffic reaches OBC-owned business logic, while preserving the current controller-oriented mock and UART paths as bounded development baselines.

#### Scenario: Current mock-radio paths stay bounded rather than becoming the end-state narrative
- **WHEN** the repository evolves the comm architecture beyond the first mock-radio slices
- **THEN** it SHALL keep the existing `mock-text`, transparent, and framed paths as governed development evidence without treating them alone as the final TT&C architecture

### Requirement: Comm Participates In The Future CSP Subsystem Topology
The future comm architecture SHALL allow `COMM` to participate as a CSP-facing subsystem alongside `EPS` and `ADCS`, instead of remaining permanently outside the internal subsystem-network story.

#### Scenario: Future COMM integration remains distinct from direct GDS and GPS
- **WHEN** the repository later adds the governed COMM CSP-facing path
- **THEN** that path SHALL remain distinct from the direct `GDS -> TCP -> OBC` development path and from the direct GPS sensor path

### Requirement: Omitted-RF TT&C Ingress Remains Distinct From The Spacecraft Internal Bus
The comm subsystem SHALL allow a first omitted-RF lab ingress path that uses a governed serial ingress into the subsystem-side comm path, and that ingress SHALL be treated as the RF-omitted boundary rather than as the spacecraft internal subsystem bus.

#### Scenario: Lab-side UART ingress does not redefine the spacecraft-side carrier
- **WHEN** a future omitted-RF TT&C change uses subsystem-side UART ingress
- **THEN** the repository SHALL still treat spacecraft-side `CAN FD` as the planned internal carrier direction for `EPS`, `ADCS`, and `COMM`
