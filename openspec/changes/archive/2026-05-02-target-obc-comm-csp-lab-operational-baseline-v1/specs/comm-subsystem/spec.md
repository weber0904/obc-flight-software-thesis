## ADDED Requirements

### Requirement: Service-Managed Subsystem COMM CSP Lab Runtime
The comm subsystem SHALL support a workspace-based lab runtime on `subsystem.local` where EPS, ADCS, and COMM node processes are managed by systemd with separate service failure boundaries.

#### Scenario: Subsystem aggregate target starts separate node services
- **WHEN** the subsystem lab runtime is installed and enabled
- **THEN** `subsystem-comm-csp-stack.target` SHALL aggregate separate EPS, ADCS, and COMM services instead of running all three long-lived processes inside one stack service
- **AND** each service SHALL have distinct systemd status and journal surfaces

#### Scenario: EPS and ADCS stay on the EPS/ADCS CAN channel
- **WHEN** the subsystem aggregate target starts EPS and ADCS services
- **THEN** EPS node `2` and ADCS node `3` SHALL run over `subsystem.local:can0` with the configured SocketCAN transport settings

#### Scenario: COMM stays on the COMM CAN channel and lab serial ingress
- **WHEN** the subsystem aggregate target starts the COMM service
- **THEN** COMM node `4` SHALL run over `subsystem.local:can1`
- **AND** it SHALL use the configured lab serial ingress device and baudrate for the RF-omitted ground path

#### Scenario: Subsystem runtime remains non-root
- **WHEN** the subsystem EPS, ADCS, and COMM services start their long-running runtime processes
- **THEN** those runtime processes SHALL run as the configured non-root workspace user for this lab baseline
- **AND** failures caused by missing workspace build outputs SHALL be reported as setup failures rather than silently falling back to root execution

### Requirement: Lab Operational COMM SocketCAN Path Reuses Existing Contract
The lab operational COMM SocketCAN baseline SHALL preserve the existing COMM node identity, service ports, stock F' framing, and bounded proof scope while moving runtime management from probes into services.

#### Scenario: COMM contract remains unchanged under services
- **WHEN** command/event/channel or housekeeping file/downlink validation runs over the service-managed lab path
- **THEN** COMM SHALL retain node `4`
- **AND** it SHALL keep services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`
- **AND** the change SHALL NOT add COMM service ports, wire layouts, custom GDS plugins, RF behavior, reliable retransmission, or arbitrary onboard file downlink
