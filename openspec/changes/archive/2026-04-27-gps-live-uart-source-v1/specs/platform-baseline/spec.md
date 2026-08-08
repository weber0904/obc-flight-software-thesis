## MODIFIED Requirements

### Requirement: Near-Term Target Serial Ownership Reflects Active Hardware Paths
The platform baseline SHALL describe direct OBC-attached GPS UART as the active near-term target GPS hardware path on `obc.local`, and it SHALL treat the former OBC-side serial comm path as deferred historical behavior until a later comm migration slice establishes a new active target hardware path.

#### Scenario: Target serial responsibilities stay explicit after GPS bring-up
- **WHEN** the repository describes the active target hardware baseline
- **THEN** it SHALL identify GPS as the active consumer of `obc.local` direct UART bring-up and SHALL keep current comm development on non-OBC-serial paths until later migration work completes
