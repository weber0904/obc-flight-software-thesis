## ADDED Requirements

### Requirement: S-band Live Packet Observability Is Auth-Gated And Quiet By Default

The current COMM runtime SHALL keep packetized S-band live `event/tlm`
observability quiet until an accepted authenticated S-band secure session is
active for the current comm-managed node-`5` path.

#### Scenario: Pre-auth S-band does not emit wholesale live packet chatter
- **WHEN** S-band is the current primary telemetry link
- **AND** no accepted authenticated S-band secure session is active
- **THEN** the runtime SHALL suppress packetized live `event/tlm` egress on the
  S-band path
- **AND** it SHALL keep command/auth closure plumbing available for the same
  path
- **AND** it SHALL NOT describe quiet startup as loss of command readiness,
  beacon duty, or file-role policy.

#### Scenario: Accepted S-band auth opens live packet observability
- **WHEN** `CommandIngressAuthority` synthesizes an accepted authenticated
  S-band session for the current comm-managed node-`5` path
- **THEN** `CommController` SHALL enable S-band live packet observability for
  that active session
- **AND** packetized live `event/tlm` SHALL remain enabled only while that
  authenticated session remains active.

#### Scenario: Session invalidation closes live packet observability
- **WHEN** the owning S-band authenticated session is revoked, role-invalidated,
  reconfigured by failover policy, or lost on restart
- **THEN** `CommController` SHALL close S-band live packet observability
  immediately
- **AND** the runtime SHALL require a new accepted S-band authentication cycle
  before live packet visibility opens again.

### Requirement: Current Observability Tiers Stay Distinct

The current baseline SHALL keep always-on critical surfaces, auth-gated live
packet observability, and bounded `GET_*` summary readback as distinct
observability tiers rather than restating all operator visibility as one
undifferentiated live telemetry surface.

#### Scenario: Always-on critical surfaces stay narrow
- **WHEN** the current baseline describes always-on observability
- **THEN** it SHALL keep that tier limited to UHF beacon plus command/auth
  closure plumbing and other already-governed minimum runtime surfaces
- **AND** it SHALL NOT restate packetized S-band `event/tlm` chatter as an
  always-on requirement by default.

#### Scenario: Bounded summary readback stays component-owned
- **WHEN** operators need status outside the current live packet tier
- **THEN** the current baseline SHALL reuse bounded component-owned `GET_*` or
  read/status command surfaces and their summary events/telemetry
- **AND** it SHALL NOT describe those bounded readbacks as a second hidden
  continuous command or telemetry plane.

### Requirement: Current Maintained Node-5 Proofs Depend On Post-Auth Visibility, Not Pre-Auth Chatter

The current maintained node-`5` S-band proof family SHALL treat live
observability dependencies as post-auth session behavior rather than as a
pre-auth always-chattering startup requirement.

#### Scenario: Maintained node-5 proofs remain compatible with auth-gated live visibility
- **WHEN** a maintained node-`5` hosted or target proof depends on S-band live
  `event/tlm` visibility
- **THEN** that proof SHALL open or reuse an accepted authenticated S-band
  session before claiming live observability
- **AND** it SHALL NOT require pre-auth broad S-band chatter as the governing
  baseline truth.
