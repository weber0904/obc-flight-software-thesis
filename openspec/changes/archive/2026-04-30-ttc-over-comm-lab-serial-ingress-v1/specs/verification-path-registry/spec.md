## ADDED Requirements

### Requirement: Physical Lab Serial Ingress Path Is Registered Separately
The verification-path registry SHALL register the physical lab serial ingress result separately from hosted PTY gateway TT&C, subsystem UART preflight, and subsystem-origin acquisition paths.

#### Scenario: Registry names the proven physical ingress boundary
- **WHEN** the lab serial ingress probe passes Stage 1
- **THEN** the registry SHALL identify the newly proven path as `fprime-cli -> GDS -> ground_ttc_gateway -> physical serial -> subsystem.local comm_csp_node -> CSP -> hosted OBC`
- **AND** it SHALL state whether the registered verdict is uplink ingress only or full bounded TT&C

#### Scenario: Registry keeps adjacent physical serial paths distinct
- **WHEN** reviewers inspect the physical lab serial ingress entry
- **THEN** the registry SHALL keep macOS-initiated UART preflight and subsystem-origin acquisition as separate supporting paths rather than treating them as the same proof boundary

#### Scenario: Failed downlink does not become a registered full TT&C path
- **WHEN** Stage 2 does not prove events and telemetry
- **THEN** the registry SHALL NOT register full physical lab serial TT&C
- **AND** it MAY register only the Stage 1 uplink ingress path if Stage 1 passed

