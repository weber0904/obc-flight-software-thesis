## ADDED Requirements

### Requirement: Subsystem COMM UART Preflight Path Is Registered
The verification-path registry SHALL include a distinct entry for the macOS-initiated physical COMM UART preflight into `subsystem.local` once the governed probe proves bounded serial request/reply traffic over that wiring.

#### Scenario: Subsystem UART preflight is separate from historical OBC UART paths
- **WHEN** reviewers check whether the new subsystem-side COMM serial wiring has been proven
- **THEN** the registry SHALL point to the subsystem COMM UART preflight evidence
- **AND** it SHALL keep the older OBC-side `/dev/serial0` external comm evidence historical rather than treating it as proof of the new subsystem-side wiring

#### Scenario: Registry does not overclaim reverse-direction acquisition
- **WHEN** reviewers check whether `subsystem.local` can initiate clean cold-first downlink into a passive macOS receiver
- **THEN** the registry SHALL treat that behavior as unproven by the subsystem COMM UART preflight unless later evidence proves it separately

#### Scenario: Subsystem UART preflight is separate from gateway-backed TT&C
- **WHEN** a later change uses the physical serial link for gateway-backed TT&C ingress
- **THEN** the registry SHALL require that later change to prove the TT&C path separately instead of reusing the UART preflight as a complete ground-link proof
