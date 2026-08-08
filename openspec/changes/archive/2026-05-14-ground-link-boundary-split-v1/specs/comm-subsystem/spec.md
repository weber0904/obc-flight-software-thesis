## ADDED Requirements

### Requirement: Ground Link Driver Stays A Byte/Backend Owner

The comm subsystem SHALL keep `GroundLinkDriver` focused on backend configuration, runtime lifecycle, byte send/receive, buffer ownership, and low-level counters/events/telemetry rather than treating it as the public mission link-state authority.

#### Scenario: Driver counters remain reviewable without owning mission policy

- **WHEN** `GroundLinkDriver` publishes TX/RX byte, chunk, or error telemetry
- **THEN** that telemetry SHALL remain the low-level driver review surface
- **AND** primary-link availability and stale-activity policy SHALL be derived elsewhere

### Requirement: Active COMM Policy Uses A Provider-Owned Link-Health Surface

The comm subsystem SHALL compute S-band and UHF ground-link health for active COMM runtime policy through a dedicated provider surface rather than by having `CommController` read raw driver stats directly.

#### Scenario: Provider runs before COMM policy

- **WHEN** the active hosted `TopCcsds` fast rate group runs
- **THEN** the ground-link health provider SHALL update per-band health views before `CommController` evaluates availability and transport-fault state

#### Scenario: COMM reads provider health views

- **WHEN** `CommController` evaluates primary-link availability or transport-fault growth
- **THEN** it SHALL consume the provider-owned per-band health view
- **AND** it SHALL NOT treat direct `GroundLinkDriver` stat polling as the authoritative mission link-state input

### Requirement: `COMM_CSP` Activity Freshness Is Reviewable

The comm subsystem SHALL define `COMM_CSP` link activity freshness for the active S-band node `5` and UHF node `6` policy paths from successful RX, successful TX, or successful status observation.

#### Scenario: Successful status observation keeps an idle COMM path healthy

- **WHEN** the active `COMM_CSP` link has no new TX or RX payload for one fast-group cycle
- **AND** the provider observes a successful link-status result during that cycle
- **THEN** the link activity age SHALL be refreshed
- **AND** the path SHALL remain available if the link is still connected

#### Scenario: Connected but stale COMM path becomes unavailable

- **WHEN** an active `COMM_CSP` path remains connected
- **BUT** it has activity age greater than one fast-group tick because RX, TX, and status observation all failed to refresh activity
- **THEN** the provider SHALL mark that path unavailable
- **AND** the reviewable reason SHALL distinguish stale activity from explicit disconnection

### Requirement: Direct TCP Remains A Connected-Only Fallback

The comm subsystem SHALL keep direct `OBC -> GDS` TCP as a connected-only development fallback instead of introducing a new keepalive protocol in this change.

#### Scenario: Idle direct TCP does not become stale only due to silence

- **WHEN** the direct TCP path remains socket-connected with no recent TX or RX payloads
- **THEN** the provider SHALL continue to treat the path as available
- **AND** it SHALL report the connected-only fallback reason rather than a stale-activity verdict

### Requirement: COMM Runtime State Exposes Per-Band Activity Age And Reasons

The comm subsystem SHALL expose per-band ground-link activity age and availability reason through reviewable COMM runtime state on the active baseline.

#### Scenario: Hosted status shows new health fields

- **WHEN** operators inspect hosted COMM runtime status
- **THEN** they SHALL be able to see S-band and UHF activity-age values
- **AND** they SHALL be able to distinguish healthy activity, connected-only fallback, explicit disconnect, and stale-activity reasons
