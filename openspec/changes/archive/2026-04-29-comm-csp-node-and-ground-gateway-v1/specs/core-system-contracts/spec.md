## MODIFIED Requirements

### Requirement: Core Contracts Distinguish Ground, Subsystem, And Sensor Paths
The core system contracts capability SHALL distinguish the direct GDS development path, the future omitted-RF TT&C comm path, the internal subsystem CSP path, and the direct GPS sensor path as separate architecture domains that later capabilities may interact with but SHALL NOT silently collapse.

#### Scenario: Future subsystem work cannot reuse the wrong domain boundary
- **WHEN** a later change extends comm, GPS, or internal CSP behavior
- **THEN** the project SHALL be able to identify which architecture domain that change belongs to without reusing an adjacent path's contract by implication

#### Scenario: Reused framing does not collapse direct GDS and COMM TT&C
- **WHEN** the repository reuses stock F' framing across both the direct GDS baseline and the first COMM-backed omitted-RF path
- **THEN** the contracts SHALL still treat those as separate architecture domains instead of treating shared framing alone as proof that the paths are interchangeable

### Requirement: Future COMM CSP Identity Is Governed Before Implementation
Before the repository implements the COMM CSP-facing subsystem path, the core system contracts capability SHALL require that COMM node identity and shared-bus participation be explicitly governed instead of being introduced ad hoc inside implementation code or scripts, and the first governed implementation SHALL reserve node `4` plus application service ports `30` through `39` for COMM-owned traffic.

#### Scenario: COMM node identity is formalized before physical-bus implementation
- **WHEN** a later change begins implementing COMM as a CSP-facing subsystem
- **THEN** that change SHALL define the COMM node identity and participation rules through governed contracts rather than by implicit script defaults alone

#### Scenario: COMM service ownership is formalized together with node identity
- **WHEN** the first governed COMM CSP path is implemented
- **THEN** the repository SHALL reserve node `4` and application service ports `30` through `39` for COMM instead of introducing those values only inside scripts, probes, or simulator source defaults
