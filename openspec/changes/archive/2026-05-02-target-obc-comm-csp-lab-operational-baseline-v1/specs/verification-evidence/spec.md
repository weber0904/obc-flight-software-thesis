## ADDED Requirements

### Requirement: Lab Target Operational Evidence
The verification evidence baseline SHALL require service-managed evidence before the lab target COMM CSP path is treated as reusable operational baseline.

#### Scenario: Evidence records OBC service migration and reboot
- **WHEN** the lab target operational probe passes
- **THEN** the evidence SHALL record the OBC package release id, installed release pointer, `obc-comm-csp-stack.service` enabled/active state, the older `obc-installed-stack.service` disabled/stopped state, and an `obc.local` reboot followed by autostart from the installed release

#### Scenario: Evidence records subsystem service boundaries
- **WHEN** the subsystem lab services are part of the verdict
- **THEN** the evidence SHALL record `subsystem-comm-csp-stack.target` state and the separate EPS, ADCS, and COMM service states and journal summaries

#### Scenario: Evidence records lab CAN provisioning
- **WHEN** CAN interface state is part of the verdict
- **THEN** the evidence SHALL record the CAN oneshot service states, configured timing, interface state, and absence of bus-off on `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`
- **AND** it SHALL state that oneshot provisioning is a lab/development helper rather than final flight OS provisioning

#### Scenario: Evidence records ground and E2E observations
- **WHEN** the full lab path is accepted
- **THEN** the evidence SHALL record ground launcher settings, command/event/channel observations, `GROUND_LINK_TX_BYTES`, OBC readback for bounded EPS and ADCS commands, live GPS UART source-mode observations, and housekeeping archive file byte matches

#### Scenario: Evidence excludes adjacent futures
- **WHEN** the lab target operational evidence is recorded
- **THEN** it SHALL explicitly exclude final flight deployment, RF behavior, no-preamble first-byte-clean behavior, reliable retransmission, arbitrary onboard file downlink, ScenarioBridge/pass automation, and final OS CAN provisioning
