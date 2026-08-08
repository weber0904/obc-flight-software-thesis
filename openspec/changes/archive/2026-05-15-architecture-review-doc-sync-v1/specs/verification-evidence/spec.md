## MODIFIED Requirements

### Requirement: Historical Housekeeping Archive Evidence
The verification evidence tree SHALL preserve the capture cadence used,
the archive-slot limits, the observed rotation behavior, the generated index
contents, the downlink commands exercised, and the final verdict for the first
housekeeping archive slice as historical evidence for the now-retired fallback
surface.

#### Scenario: Housekeeping archive slice is reviewable
- **WHEN** the housekeeping archive change completes
- **THEN** reviewers SHALL be able to inspect hosted validation steps covering archive capture, slot rotation, index generation, and commanded index or slot downlink from the repository evidence tree

### Requirement: Lab Target Operational Evidence
The verification evidence baseline SHALL require service-managed evidence before
the lab target COMM CSP path is treated as reusable operational baseline. The
historical lab target evidence may remain HK-fallback based, but current or
future mission-history file/downlink claims over that path SHALL cite official
`.fdp` / `DpCatalog` byte-match evidence instead of treating the historical HK
fallback byte matches as current proof.

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
- **THEN** the evidence SHALL record ground launcher settings, command/event/channel observations, `GROUND_LINK_TX_BYTES`, OBC readback for bounded EPS and ADCS commands, and live GPS UART source-mode observations
- **AND** any current mission-history file/downlink claim SHALL additionally cite official `.fdp` / `DpCatalog` byte matches for the path under review

#### Scenario: Evidence excludes adjacent futures
- **WHEN** the lab target operational evidence is recorded
- **THEN** it SHALL explicitly exclude final flight deployment, RF behavior, no-preamble first-byte-clean behavior, reliable retransmission, arbitrary onboard file downlink, ScenarioBridge/pass automation, and final OS CAN provisioning
