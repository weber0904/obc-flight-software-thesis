## MODIFIED Requirements
### Requirement: Comms Verification Modes

The subsystem SHALL support validation through TCP mock and PTY-backed virtual UART paths, SHALL classify true hardware UART or radio validation as `Blocked-HW` when the necessary hardware is unavailable, and SHALL provide a hosted comm mock executable that the integrated software-only OBC runtime can use without test-only harness code.

#### Scenario: Hosted OBC runtime uses the same comm abstractions as tests
- **WHEN** the software-only OBC runtime launches with the first external comm path
- **THEN** `RadioController` and `UartDriver` SHALL operate through the shared hosted byte-stream abstractions against a standalone mock server rather than through test-only glue code
