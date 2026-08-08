## ADDED Requirements

### Requirement: Command Ingress Source Index Evidence Is Reviewable
The verification evidence SHALL record the configured ingress source-index behavior introduced by `command-ingress-source-index-v1`.

#### Scenario: Component tests prove multi-port behavior
- **WHEN** the change is closed out
- **THEN** evidence SHALL list component tests covering configured port `0`, configured port `1`, unconfigured port fail-closed behavior, legacy `configure(config)` clearing semantics, and context preservation.

#### Scenario: Hosted probe proof is scoped to port zero
- **WHEN** hosted probe evidence is recorded
- **THEN** it SHALL state that current hosted default CCSDS and legacy ComFprime topologies wire only authority ingress index `0`
- **AND** it SHALL avoid claiming hosted proof for ingress port `1` or simultaneous S-band/UHF routed command ingress.
