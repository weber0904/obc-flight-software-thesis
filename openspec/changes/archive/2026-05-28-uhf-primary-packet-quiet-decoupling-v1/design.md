## Context

The active baseline already proves three separate UHF truths:

- `uhf-backup` is bounded allowlisted backup ingress, not beacon-only.
- official file/data-product downlink remains formal even when live packet
  noise is suppressed on the formal UHF path.
- UHF beacon suppress/runtime is COMM-owned and begins only after accepted
  authenticated `SESSION_OPEN(seq0)` with later same-session accepted UHF
  activity refreshing the active window.

The remaining problem is that current runtime naming and spec wording still
describe formal UHF packet quiet and UHF beacon suppress as one active
command-session quiet concept. The code already keeps file routing separate, so
the remaining work is to separate packet quiet from beacon suppress explicitly
in runtime policy, naming, tests, and reviewable documentation.

## Goals

- make formal UHF live packet suppression primary-band-driven
- keep UHF beacon suppress accepted-session-driven
- preserve official file/data-product downlink while UHF primary packet quiet
  is active
- keep diagnostic quiet as the stronger probe-only hard quiet
- make the resulting semantics explicit in runtime state, tests, docs, and
  verification paths

## Non-Goals

- no gateway role changes
- no simultaneous dual-link orchestration
- no second-GDS productization
- no UHF reliable transfer, ARQ, NACK, or CFDP expansion
- no RF closure
- no broad target non-quiet closure beyond the existing bounded evidence

## Runtime Semantics

### UHF Primary Packet Quiet

Formal UHF primary packet quiet is a primary-band policy.

It is active when:

- `currentPrimaryBand == UHF`

It suppresses:

- live `event/tlm` packet egress on the formal UHF path

It does not suppress:

- official file/data-product downlink routing
- beacon emission by itself

It clears when:

- the current primary band is no longer UHF

### UHF Beacon Suppress / Runtime

UHF beacon suppress remains a session-owner policy.

It starts only after:

- accepted authenticated qualifying UHF `SESSION_OPEN(seq0)`

It refreshes only after:

- later accepted same-session UHF command activity, including read/status

It clears on:

- inactivity timeout
- session revoke
- session replace
- role invalidation or ownership invalidation

Qualifying UHF suppress sessions remain:

- `uhf-backup`
- `uhf-primary-after-failover`

This means a valid UHF backup session may still suppress beacon while S-band
remains primary, but packet quiet remains off in that situation because packet
quiet depends only on the current primary band.

## Naming And State Surface

The old packet-side `session quiet` naming is no longer honest once packet
quiet no longer depends on accepted-session ownership.

This change renames the packet-side concept to an explicit UHF-primary-driven
name:

- `setSessionQuietPacketEgressForRuntime(...)` becomes
  `setUhfPrimaryPacketQuietForRuntime(...)`
- `getSessionQuietPacketEgressForRuntime()` becomes
  `getUhfPrimaryPacketQuietForRuntime()`
- `m_sessionQuietPacketEgress` becomes `m_uhfPrimaryPacketQuiet`

Beacon fields remain under the current `uhfBeaconSuppress*` family.

`CommRuntimeState` gains:

- `uhfPrimaryPacketQuietActive`

This lets reviewers observe:

- packet quiet state directly
- beacon suppress state directly

without inferring one from the other.

## Owner Boundary

- `CommController` remains the policy owner for:
  - current primary command/telemetry/file band
  - UHF primary packet quiet state
  - UHF beacon suppress/runtime state
- `CommEgressMux` remains the egress enforcement point for:
  - packet routing
  - file routing
  - packet quiet enforcement
  - diagnostic quiet enforcement

`CommEgressMux` must not gain gateway, authority, session, or relay policy
ownership.

## Verification Shape

The current hosted beacon suppress/runtime path remains separate and unchanged
in purpose.

This change adds a separate hosted verification path for UHF primary packet
quiet that proves:

- entering `uhf-primary-after-failover` suppresses live packet egress before
  accepted `SESSION_OPEN(seq0)`
- entering UHF primary alone does not start beacon suppress
- accepted qualifying UHF `SESSION_OPEN(seq0)` still starts beacon suppress
- accepted same-session activity still refreshes beacon suppress timeout
- official file/data-product downlink remains formal and routable while packet
  quiet is active

## Risks And Trade-Offs

- **[Risk] reviewers may still conflate packet quiet with beacon suppress**
  - Mitigation: rename packet-side runtime methods/fields now and add direct
    runtime state plus dedicated hosted proof.
- **[Risk] UHF backup beacon suppress may look surprising once packet quiet is
  primary-band-driven**
  - Mitigation: document explicitly that beacon suppress qualifying sessions
    still include `uhf-backup`, while packet quiet does not.
- **[Risk] packet quiet could be misread as making file/downlink disposable**
  - Mitigation: keep file routing implementation unchanged and prove it in UT
    and hosted evidence.
