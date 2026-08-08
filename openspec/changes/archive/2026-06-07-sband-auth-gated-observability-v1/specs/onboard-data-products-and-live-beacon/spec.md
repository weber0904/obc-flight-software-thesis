## ADDED Requirements

### Requirement: Live Operational Visibility Distinguishes Beacon, Auth-Gated Live Packets, And Bounded Summary Readback

The onboard state data system SHALL describe nominal live operational
visibility as three distinct current surfaces: always-on live beacon,
auth-gated packetized live observability, and bounded `GET_*` summary readback.

#### Scenario: Mission-history wording does not collapse current observability tiers
- **WHEN** the repository describes current live versus stored HK/state roles
- **THEN** it SHALL keep live beacon as a no-ACK current-health broadcast
- **AND** it SHALL describe packetized S-band `event/tlm` as auth-gated live
  operational visibility rather than as unconditional startup chatter
- **AND** it SHALL describe bounded `GET_*` summary readback as on-demand live
  visibility rather than mission-history storage.

#### Scenario: Bounded readback does not become a second storage path
- **WHEN** component-owned `GET_*` or read/status commands emit summary
  events/telemetry
- **THEN** those summaries SHALL remain operator-facing live readback surfaces
- **AND** they SHALL NOT be described as replacing official HK `.fdp`
  mission-history products or as opening a new generic stored-history path.
