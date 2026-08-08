## MODIFIED Requirements

### Requirement: Comms Verification Modes
The subsystem SHALL support validation through TCP mock and PTY-backed virtual UART paths, SHALL classify true hardware UART or radio validation as `Blocked-HW` when the necessary hardware path is still unavailable, SHALL provide a hosted comm mock executable that the integrated software-only OBC runtime can use without test-only harness code, SHALL expose a hosted ground path that uses `Drv::TcpClient` to connect the OBC deployment to `fprime-gds`, and SHALL provide a repo-local Raspberry Pi target integration path where the same controller logic runs against either TCP mock or an explicit UART device path selected by profile configuration.

#### Scenario: Hosted ground path uses the documented TCP client link
- **WHEN** the hosted OBC deployment runs against `fprime-gds`
- **THEN** the deployment SHALL bring up the F' command/event/tlm stack and connect to the documented IP adapter port through `Drv::TcpClient`

#### Scenario: Raspberry Pi target stack reuses shared comm logic
- **WHEN** the project launches the `integ-rpi` profile on a Raspberry Pi target
- **THEN** the external link SHALL be selectable through runtime transport settings such as TCP mock or UART device path while `CommController`, `RadioController`, and `UartDriver` keep the same controller-layer behavior as the hosted profile
