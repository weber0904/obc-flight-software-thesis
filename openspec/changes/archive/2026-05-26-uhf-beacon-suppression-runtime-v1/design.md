## Context

The current baseline already freezes these COMM policy points:

- accepted `SESSION_OPEN(seq0)` is the current UHF operational session boundary
- `CommController` owns beacon suppress/resume policy
- suppress starts at accepted `SESSION_OPEN(seq0)`
- resume happens after bounded inactivity timeout

What remains open is runtime closure. `BeaconPublisher` currently emits on fixed cadence without session awareness, and `CommController` does not yet own any suppress state machine or beacon gating surface.

## Goals

- make UHF beacon suppress/resume a real runtime behavior on active `TopCcsds`
- keep suppress ownership in `CommController`
- define precise inactivity timeout semantics as fixed `60` scheduler ticks
- make suppress state and transitions reviewable through runtime status, telemetry, and events
- prove the behavior on one hosted node-6 path and one target CAN quiet-UHF node-6 path

## Non-Goals

- no simultaneous dual-link orchestration
- no UHF reliable transfer, ARQ/NACK, or CFDP
- no gateway redesign
- no radio metrics redesign
- no RF or non-quiet-UHF closure
- no broader UHF handshake state machine beyond accepted `SESSION_OPEN(seq0)`

## Runtime Rule

The v1 runtime rule is:

1. suppress starts only after an accepted authenticated UHF `SESSION_OPEN(seq0)`
2. any later accepted authenticated UHF command in that same active session, including read/status traffic, refreshes the inactivity window
3. the inactivity timeout is fixed at `60` scheduler ticks on the current `1 Hz` baseline
4. suppress clears immediately when the owning UHF session is revoked, replaced, or invalidated by role switch/failover
5. resume occurs when the active suppress window reaches `0` remaining ticks

The following do not trigger suppress:

- raw link acquisition alone
- unrelated S-band or non-UHF activity
- malformed, invalid, rejected, or non-accepted `SESSION_OPEN`
- denied commands that never become accepted UHF session activity

## Owner Model

### CommandIngressAuthority

- remains owner of envelope parsing, auth, lifecycle, and sequence acceptance
- emits bounded runtime observer callbacks for:
  - accepted session open/replace
  - accepted post-open command activity
  - session revoke/clear

### CommController

- implements the observer
- owns suppress active state, owner session identity, last accepted sequence, fixed timeout ticks, and remaining ticks
- filters observer callbacks down to qualifying UHF session activity only
- drives the bounded beacon gate surface
- clears suppress immediately on session invalidation or timeout expiry

### BeaconPublisher

- remains session-agnostic
- gains only a bounded runtime suppression gate
- does not interpret session roles, command classes, or lifecycle events

## Interfaces

### Runtime observer

Add a bounded runtime observer interface from `CommandIngressAuthority` to `CommController` with callbacks for:

- accepted session open
- accepted command activity
- session revoke

Callback payload must include ingress port, link identity, link role, session id, and accepted sequence number so `CommController` can bind suppress state to one explicit UHF session instance.

### Beacon gate

Add a bounded runtime suppress control interface that `BeaconPublisher` implements and `CommController` drives. The gate is boolean and independent from cadence configuration.

### Comm runtime state and observability

Extend the runtime state and status surfaces with:

- suppress active
- suppress owner ingress port
- suppress owner link role
- suppress owner session id
- suppress last accepted sequence
- suppress remaining ticks
- suppress timeout ticks

Add transition events for:

- suppress started
- suppress refreshed
- suppress cleared by revoke/replace/role invalidation
- suppress resumed by inactivity timeout

## State Machine

### Start

- on accepted UHF `SESSION_OPEN(seq0)`, enter suppress active
- owner session becomes the accepted `(port, role, session_id)`
- remaining ticks set to `60`

### Refresh

- on later accepted authenticated UHF command activity that matches the owning session, reset remaining ticks to `60`
- duplicate `SESSION_OPEN` that is rejected does nothing
- accepted replacement `SESSION_OPEN(seq0)` starts a new owner session and resets the window

### Clear

- on explicit revoke
- on role/profile reconfiguration that revokes the old session
- on recovery failover that revokes or invalidates the owning session

### Timeout resume

- decrement remaining ticks once per `CommController.schedIn`
- when the counter reaches `0`, clear suppress and resume beacon emission immediately

## Verification Design

### Focused local coverage

- `CommController` UT:
  - suppress starts on accepted UHF open
  - accepted UHF read/status refreshes remaining ticks
  - timeout resumes
  - duplicate open rejection does not refresh
  - accepted replacement open rebinds owner session
  - revoke clears immediately
  - role switch/failover clears immediately
  - non-UHF or non-qualifying activity does not start suppress
- `CommandIngressAuthority` UT:
  - observer fires only on accepted lifecycle/activity transitions
  - rejected session open and rejected command paths do not notify
- `BeaconPublisher` UT:
  - suppressed scheduler ticks stay silent without surfacing configuration/error events
  - unsuppress resumes normal emission and sequence progression

### Hosted proof

- add a dedicated hosted node-6 probe with probe-owned beacon capture
- prove:
  - baseline beacon exists before session open
  - accepted `SESSION_OPEN(seq0)` starts suppress
  - accepted UHF read/status traffic refreshes the window
  - beacon resumes after `60` ticks of inactivity
  - invalid/rejected and non-qualifying cases do not suppress

### Target proof

- add a dedicated target CAN quiet-UHF node-6 wrapper
- if needed, extend subsystem UHF launch/service surfaces with an optional probe-owned beacon capture serial device
- prove the same start/hold/resume rule with capture plus journal markers

## Risks And Controls

- Risk: negative cases could accidentally count denied traffic as refresh
  - Control: observer emits only after acceptance; `CommController` filters by owning UHF session
- Risk: suppress could outlive a revoked session
  - Control: immediate clear on revoke/replace/role invalidation is explicit in the state machine
- Risk: target proof could silently drift into non-quiet-UHF claims
  - Control: keep target evidence scoped to the current CAN quiet-UHF node-6 path and restate non-claims in the record
