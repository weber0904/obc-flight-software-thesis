## MODIFIED Requirements

### Requirement: Beacon Suppress And Resume Policy Belongs To CommController

The current COMM runtime SHALL assign UHF beacon suppress and resume ownership
to `CommController`, SHALL start suppress only after an accepted authenticated
UHF `SESSION_OPEN(seq0)` on the qualifying comm-managed ingress, SHALL refresh
the suppress window only on later accepted authenticated UHF command activity
for that same active session, and SHALL resume beaconing only after immediate
session invalidation or a bounded inactivity timeout.

#### Scenario: Beacon publisher does not own command-session arbitration
- **WHEN** reviewers inspect beacon/session runtime behavior
- **THEN** `BeaconPublisher` SHALL remain the bounded cadence and encode owner
- **AND** it SHALL NOT be described as the authority that decides when command
  session activity suppresses or resumes UHF beaconing

#### Scenario: Suppress start requires accepted SESSION_OPEN(seq0)
- **WHEN** raw UHF link acquisition, unrelated path traffic, invalid traffic,
  rejected traffic, or any non-accepted `SESSION_OPEN` occurs
- **THEN** the runtime SHALL NOT enter suppress because of that traffic alone
- **AND** the suppress-start boundary SHALL remain the accepted authenticated
  UHF `SESSION_OPEN(seq0)` for the qualifying active session

#### Scenario: Accepted UHF session activity refreshes the active window
- **WHEN** the owning UHF session is already suppressing beacon emission
- **AND** a later accepted authenticated UHF command in that same active
  session succeeds, including read or status traffic
- **THEN** `CommController` SHALL refresh the bounded inactivity window for the
  owning session

#### Scenario: Timeout semantics stay fixed and reviewable on the current baseline
- **WHEN** reviewers inspect the active suppress window semantics
- **THEN** the runtime SHALL expose the timeout as fixed `60` scheduler ticks
  on the current `1 Hz` baseline
- **AND** it SHALL expose suppress-active state, owner ingress/role/session,
  last accepted sequence, remaining ticks, and transition reason as reviewable
  runtime state

#### Scenario: Session invalidation clears suppress immediately
- **WHEN** the owning UHF session is revoked, replaced, or invalidated by role
  switch or COMM failover handling
- **THEN** `CommController` SHALL clear suppress immediately
- **AND** resume SHALL NOT wait for the remaining inactivity window to expire

### Requirement: UHF Primary Packet Quiet Suppresses Live Packet Noise But Preserves Official File Downlink

The current COMM runtime SHALL treat formal UHF live packet suppression as a
primary-band-driven policy, SHALL suppress live `event/tlm` packet egress on
the formal UHF path whenever the current primary band is UHF, SHALL keep UHF
beacon suppress/runtime as a separate accepted-session-driven policy, and SHALL
NOT reuse UHF primary packet quiet to disable official file/data-product
downlink routing.

#### Scenario: Entering UHF primary immediately suppresses live packet egress
- **WHEN** `uhf-primary-after-failover` becomes the current primary band
- **THEN** the formal UHF path SHALL suppress live `event/tlm` packet egress
  immediately
- **AND** that suppression SHALL NOT wait for accepted `SESSION_OPEN(seq0)` to
  begin

#### Scenario: UHF primary packet quiet does not start beacon suppress by itself
- **WHEN** UHF becomes the current primary band before any accepted qualifying
  UHF `SESSION_OPEN(seq0)` exists
- **THEN** formal UHF live packet egress SHALL already be suppressed
- **AND** UHF beacon suppress SHALL remain inactive until accepted qualifying
  session ownership starts it

#### Scenario: Leaving UHF primary clears packet quiet independently
- **WHEN** the current primary band is no longer UHF
- **THEN** UHF primary packet quiet SHALL clear even if beacon suppress was
  never started
- **AND** beacon suppress clear semantics SHALL remain governed by the existing
  session invalidate or timeout rules rather than by packet quiet alone

#### Scenario: Official file downlink remains formal capability during UHF primary packet quiet
- **WHEN** UHF primary packet quiet is active
- **THEN** official file/data-product downlink routing SHALL remain available
  as a formal spacecraft capability
- **AND** the runtime SHALL NOT treat file/data-product downlink as disposable
  background packet noise merely because live `event/tlm` suppression is active
