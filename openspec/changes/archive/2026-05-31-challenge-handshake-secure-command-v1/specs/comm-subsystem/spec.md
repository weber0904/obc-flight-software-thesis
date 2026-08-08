## MODIFIED Requirements

### Requirement: Accepted SESSION_OPEN(seq0) Is The Current UHF Operational Session Boundary

The comm subsystem SHALL make accepted UHF authorization completion for the
UHF secure service the current UHF operational command-session boundary for the
new secure path, and accepted `SESSION_OPEN` with `sequence_number = 0` SHALL
remain the boundary only for legacy command envelope v1 traffic.

#### Scenario: Link acquisition alone is not a secure command-session boundary
- **WHEN** the UHF link is electrically present, bytes are exchanged, or a
  first non-lifecycle command succeeds on an adjacent path
- **THEN** the repository SHALL NOT describe that fact alone as the current
  UHF operational secure command-session boundary.

#### Scenario: UHF auth success is the new secure boundary
- **WHEN** `SecureLinkAuthorizer` accepts a valid `RESPONSE` for
  `ServiceID = 2` on the relevant comm-managed ingress
- **AND** `CommandIngressAuthority` synthesizes the runtime opened-session
  state for that auth grant
- **THEN** the repository SHALL treat that auth completion as the current UHF
  operational secure command-session boundary for the new path.

### Requirement: Beacon Suppress And Resume Policy Belongs To CommController

The current COMM runtime SHALL assign UHF beacon suppress and resume ownership
to `CommController`, SHALL start suppress only after accepted UHF secure
authorization completion synthesizes the opened-session state for the owning
session, SHALL refresh the suppress window only on later accepted
authenticated UHF secure command activity for that same active session, and
SHALL resume beaconing only after immediate session invalidation or a bounded
inactivity timeout.

#### Scenario: Suppress start requires accepted UHF auth completion
- **WHEN** raw UHF link acquisition, unrelated path traffic, invalid traffic,
  rejected traffic, or any non-accepted handshake exchange occurs
- **THEN** the runtime SHALL NOT enter suppress because of that traffic alone
- **AND** the suppress-start boundary SHALL remain accepted UHF auth
  completion for the qualifying active secure session.

#### Scenario: Accepted UHF secure command activity refreshes the active window
- **WHEN** the owning UHF secure session is already suppressing beacon
  emission
- **AND** a later accepted authenticated UHF secure command in that same
  active session succeeds, including read or status traffic
- **THEN** `CommController` SHALL refresh the bounded inactivity window for
  the owning session.

#### Scenario: Role invalidation clears suppress immediately
- **WHEN** the owning UHF secure session is invalidated by role switch or COMM
  failover handling
- **THEN** `CommController` SHALL clear suppress immediately
- **AND** the runtime SHALL require a new successful UHF authorization cycle
  before suppress may start again.

### Requirement: UHF Primary Packet Quiet Suppresses Live Packet Noise But Preserves Official File Downlink

The current COMM runtime SHALL keep UHF primary packet quiet as a band-state
mechanism independent from secure authorization completion and SHALL NOT reuse
the new secure auth boundary to redefine file/downlink ownership.

#### Scenario: Entering UHF primary alone still does not start beacon suppress
- **WHEN** UHF becomes the current primary band
- **AND** no accepted qualifying UHF auth completion has started a beacon
  suppress owner
- **THEN** the runtime SHALL leave beacon suppress inactive
- **AND** it SHALL keep packet quiet and beacon suppress reviewable as
  separate mechanisms.
