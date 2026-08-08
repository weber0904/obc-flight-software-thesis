## ADDED Requirements
### Requirement: Hosted Probe GDS CLI Startup Order

The `hosted-probe-workflow` skill SHALL define the standard startup order for repository-owned hosted probes that depend on headless GDS, `fprime-cli` listeners, gateway processes, and hosted OBC runtimes.

#### Scenario: Probe observes GDS CLI events or channels

- **WHEN** a hosted probe expects `fprime-cli events` or `fprime-cli channels` output as part of its verdict
- **THEN** the skill SHALL direct the agent to start headless GDS, wait for configured GDS readiness, start the relevant `fprime-cli` listeners, allow a short listener settle interval, start gateways, COMM nodes, and target simulators before the OBC/runtime path, and only then start the OBC/runtime process that produces the traffic under observation
- **AND** the skill SHALL warn the agent not to replace missed listener output with OBC runtime logs when the formal verdict requires ground-side CLI observation

#### Scenario: Probe allocates local GDS or TTS ports

- **WHEN** a hosted probe chooses local GDS, TTS, or adjacent runtime ports
- **THEN** the skill SHALL prefer a real socket bind check over process-list checks alone and SHALL tell the agent to avoid parallel runs that share GDS, TTS, ZMQ, serial, or runtime-root resources
