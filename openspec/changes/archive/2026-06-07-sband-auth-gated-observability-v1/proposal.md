## Why

The current maintained node-`5` S-band baseline still treats packetized live
`event/tlm` observability as effectively always-on because `CommEgressMux`
routes the whole live packet stream to the current primary telemetry link
without an S-band auth gate. That behavior no longer matches the repository's
intended secure baseline: quiet-by-default startup, auth-gated live
observability, always-on low-rate critical surfaces, and bounded `GET_*`
summary readback.

This change narrows that gap without redesigning secure auth, command
authority, telemetry schemas, or UHF operator behavior. It formalizes the
observability tiers already emerging in the repo and makes node-`5` S-band
live packet visibility open only during an accepted authenticated session.

## What Changes

- Freeze the observability audit in the change artifacts:
  - current node-`5` S-band live observability is wholesale packetized
    `event/tlm` routing with no S-band auth gate
  - governed UHF behavior today is UHF primary packet quiet plus UHF beacon
    suppress
  - beacon remains a UHF no-ACK broadcast surface, not an S-band surface
- Add COMM-owned runtime policy for S-band live observability so packetized
  S-band `event/tlm` remains quiet until an accepted S-band secure-auth session
  is active, then closes again on revoke, role invalidation, failover
  reconfiguration, or restart.
- Formalize three observability tiers for the current baseline:
  - always-on critical: UHF beacon plus command/auth closure plumbing
  - auth-gated live: current packetized S-band `event/tlm`
  - `GET_*`-driven summary: existing bounded read/status commands and their
    component-owned summary events/telemetry
- Extend `CommEgressMux` with a reusable live-packet gate so S-band live packet
  suppression is policy-owned by `CommController` rather than inferred from
  transport adjacency.
- Add bounded COMM observability telemetry/events and focused hosted/target
  probes that prove pre-auth quiet, post-auth live visibility, and bounded
  summary readback without turning `GET_*` into a second command plane.
- Update current docs and operator runbooks so maintained node-`5` truth is
  "quiet until accepted secure auth, then live observability during the
  authenticated session."

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: add auth-gated S-band live observability policy and define
  the current tier boundary between always-on critical surfaces, auth-gated
  packetized live observability, and bounded `GET_*` summary readback.
- `interface-contract-index`: require `docs/interfaces.md` to describe the
  current observability tiers, S-band auth-gated live packet boundary, and the
  distinction between live packet visibility and bounded summary readback.
- `onboard-data-products-and-live-beacon`: redefine nominal live operational
  visibility so beacon, auth-gated live packet visibility, and `GET_*` summary
  readback remain distinct from mission-history storage.
- `verification-path-registry`: register the hosted and target node-`5`
  observability-governance proofs as bounded node-`5` S-band paths distinct
  from UHF quiet, UHF beacon suppress, and unrelated telemetry-heavy adjunct
  records.

## Impact

- Affected code:
  - `OBC/Components/CommController`
  - `OBC/Components/CommEgressMux`
  - focused UT coverage for both components
- Affected docs:
  - `docs/interfaces.md`
  - `docs/architecture/current-development-architecture.md`
  - maintained hosted/target node-`5` operator runbooks
  - new `evidence/records/` evidence record
- Affected proof surfaces:
  - new hosted node-`5` observability-governance probe
  - new target node-`5` observability-governance probe
  - regression rerun for hosted official sequencing/system-resources and the
    target secure-auth control boundary
- Non-goals:
  - no secure-auth redesign
  - no command-authority redesign
  - no generic telemetry-schema redesign
  - no new UHF operator behavior in this change
