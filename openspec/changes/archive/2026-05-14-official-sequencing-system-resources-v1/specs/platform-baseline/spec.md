## ADDED Requirements

### Requirement: Active TopCcsds Includes Official Sequencing and System Resource Services

The active `TopCcsds` baseline SHALL integrate official `Svc::CmdSequencer`, `Svc::SeqDispatcher`, and `Svc::SystemResources` as governed runtime services instead of adding a repo-native timed-command table.

#### Scenario: Official services are part of active topology

- **WHEN** the active topology is built and initialized
- **THEN** it SHALL instantiate two `CmdSequencer` instances, one `SeqDispatcher`, and one `SystemResources`
- **AND** it SHALL wire sequencer-emitted command traffic through dedicated `CmdDispatcher.seqCmdBuff` / `seqCmdStatus` indices distinct from the external SBAND/UHF ingress slot
- **AND** it SHALL keep one dispatcher sequence slot reserved for repo-owned internal sequence control traffic

### Requirement: Active Sequencing Timing Truth Is Bounded By Hosted Base Tick

The active official sequencing baseline SHALL state its timing truth relative to the configured deployment tick rather than mission time.

#### Scenario: Hosted timing truth is bounded and explicit

- **WHEN** evidence or docs claim absolute or relative sequence timing behavior
- **THEN** they SHALL identify the configured hosted base tick
- **AND** they SHALL state the worst-case dispatch latency bound in ticks
- **AND** they SHALL NOT claim GPS mission-time scheduling or RTC proof
