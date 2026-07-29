## MODIFIED Requirements
### Requirement: Comms Verification Modes

The subsystem SHALL support validation through TCP mock and PTY-backed virtual UART paths, SHALL classify true hardware UART or radio validation as `Blocked-HW` when the necessary hardware is unavailable, SHALL provide a hosted comm mock executable that the integrated software-only OBC runtime can use without test-only harness code, and SHALL expose a hosted ground path that uses `Drv::TcpClient` to connect the OBC deployment to `fprime-gds`.

#### Scenario: Hosted ground path uses the documented TCP client link
- **WHEN** the hosted OBC deployment runs against `fprime-gds`
- **THEN** the deployment SHALL bring up the F' command/event/tlm stack and connect to the documented IP adapter port through `Drv::TcpClient`
