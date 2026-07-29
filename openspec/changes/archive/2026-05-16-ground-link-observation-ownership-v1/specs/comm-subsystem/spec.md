## ADDED Requirements

### Requirement: Ground Link Observation Contract Is OBC-Owned

The comm subsystem SHALL keep provider-facing ground-link observation types in the OBC-owned runtime surface rather than in the simulator/backend-owned implementation header.

#### Scenario: Provider consumes OBC-owned observation types

- **WHEN** `GroundLinkHealthProvider` reads a ground-link observation
- **THEN** the observation type SHALL come from the OBC-owned ground-link runtime contract
- **AND** simulator/backend code SHALL only implement and populate that contract
- **AND** the provider-facing observation contract SHALL NOT expose raw CSP target-node identity

### Requirement: Link Health Semantics Are Explicit

The comm subsystem SHALL publish explicit ground-link health semantics with each observation so the provider does not infer active mission policy from raw backend node identifiers.

#### Scenario: Backend publishes configured semantics

- **WHEN** a COMM CSP backend publishes a ground-link observation
- **THEN** the observation SHALL use the health semantics supplied by runtime/topology configuration
- **AND** the backend SHALL NOT re-infer provider policy from raw node ID during observation publication

#### Scenario: Active COMM CSP nodes use active semantics

- **WHEN** a COMM CSP backend represents S-band node `5` or UHF node `6`
- **THEN** its observation SHALL use active COMM CSP health semantics
- **AND** the provider SHALL apply active link activity freshness and transport-growth policy

#### Scenario: Generic node 4 uses connected-only compatibility semantics

- **WHEN** a COMM CSP backend represents generic compatibility node `4`
- **THEN** its observation SHALL use connected-only fallback health semantics
- **AND** the provider SHALL NOT mark idle silence as stale activity
- **AND** the provider SHALL NOT report transport-growth faults from connected-only fallback observations

#### Scenario: Connected-only fallback remains available by connection state

- **WHEN** a connected direct-TCP or node `4` compatibility observation is evaluated
- **THEN** the provider SHALL report the link available from connection state
- **AND** the availability reason SHALL identify connected-only fallback

#### Scenario: Disabled observations are unavailable

- **WHEN** a disabled/no-backend observation is evaluated
- **THEN** the provider SHALL report the link unavailable

#### Scenario: Unsupported COMM CSP nodes do not become fallback links

- **WHEN** a COMM CSP backend represents a node other than generic node `4`, S-band node `5`, or UHF node `6`
- **THEN** its observation SHALL use disabled health semantics
- **AND** the provider SHALL NOT report connected-only fallback availability

### Requirement: CommController Has No Driver Type Coupling

The comm subsystem SHALL keep `CommController` policy evaluation dependent on the ground-link health provider rather than on direct `GroundLinkDriver` pointers.

#### Scenario: COMM policy evaluates provider state only

- **WHEN** `CommController` is configured for runtime operation
- **THEN** its ground-link policy dependency SHALL be the health provider surface
- **AND** it SHALL NOT store direct S-band or UHF `GroundLinkDriver` pointers
