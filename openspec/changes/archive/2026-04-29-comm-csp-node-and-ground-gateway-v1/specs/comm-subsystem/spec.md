## ADDED Requirements

### Requirement: First COMM CSP Path Bridges Bounded Ground-Link Chunks
The comm subsystem SHALL provide a first governed CSP-facing `COMM` node that bridges bounded omitted-RF ground-link byte chunks between the lab-side serial ingress and the spacecraft-side internal CSP bus while preserving the existing controller-oriented mock and UART comm path as a separate baseline.

#### Scenario: COMM node bridges serial ingress and internal CSP
- **WHEN** the repository runs the first gateway-backed omitted-RF TT&C path
- **THEN** the subsystem-side `COMM` process SHALL accept bounded stock F' framed byte chunks from the governed lab-side serial ingress
- **AND** it SHALL exchange those chunks with `OBC` over COMM-owned CSP services without redefining the older controller-oriented external comm path as the same architecture domain

## MODIFIED Requirements

### Requirement: Comm Participates In The Future CSP Subsystem Topology
The future comm architecture SHALL allow `COMM` to participate as a CSP-facing subsystem alongside `EPS` and `ADCS`, and the first governed implementation SHALL assign `COMM` to node `4` with reserved COMM-owned application service ports `30` through `39`.

#### Scenario: Future COMM integration remains distinct from direct GDS and GPS
- **WHEN** the repository later adds the governed COMM CSP-facing path
- **THEN** that path SHALL remain distinct from the direct `GDS -> TCP -> OBC` development path and from the direct GPS sensor path

#### Scenario: First COMM CSP slice reserves stable identity and service ownership
- **WHEN** the first governed COMM CSP path is implemented
- **THEN** the repository SHALL keep node `4` and application service ports `30` through `39` reserved for COMM-owned traffic instead of reusing EPS or ADCS port ranges

### Requirement: Omitted-RF TT&C Ingress Remains Distinct From The Spacecraft Internal Bus
The comm subsystem SHALL allow a first omitted-RF lab ingress path that uses a governed serial ingress into the subsystem-side comm path, and that ingress SHALL be treated as the RF-omitted boundary rather than as the spacecraft internal subsystem bus.

#### Scenario: Lab-side UART ingress does not redefine the spacecraft-side carrier
- **WHEN** a future omitted-RF TT&C change uses subsystem-side UART ingress
- **THEN** the repository SHALL still treat spacecraft-side `CAN FD` as the planned internal carrier direction for `EPS`, `ADCS`, and `COMM`

#### Scenario: First COMM CSP slice keeps legacy external comm baseline separate
- **WHEN** the first gateway-backed COMM path is validated through subsystem-side serial ingress
- **THEN** the repository SHALL continue to describe the older mock, transparent, and framed external comm paths as separate governed development baselines instead of folding them into the COMM CSP proof
