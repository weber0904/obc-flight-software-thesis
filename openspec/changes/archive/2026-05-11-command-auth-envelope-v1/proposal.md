## Why

The active command ingress path now has authority policy, envelope metadata, explicit session lifecycle, and strict-monotonic sequence enforcement, but it is still not authenticated. A packet that matches the current envelope/session/sequence shape can still reach runtime policy evaluation without source-bound integrity verification, so the repo cannot yet claim an authenticated command ingress foundation.

## What Changes

- Extend command envelope v1 with authenticated source identity, key-slot selection, and HMAC verification.
- Make `CommandIngressAuthority` verify auth before authority, lifecycle, sequence, or dispatch decisions for enveloped traffic.
- Bind authenticated input to source identity, session ID, sequence number, protocol version, and full inner command payload.
- Keep legacy non-envelope routed commands as an explicit compatibility path outside the authenticated claim.
- Add dedicated auth-failure events, telemetry, component coverage, and a hosted authenticated-envelope probe.
- Update canonical verification and architecture documents to reflect the new authenticated-ingress baseline and its bounded non-claims.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: add authenticated envelope requirements, auth-before-policy ordering, and explicit auth/lifecycle/sequence state-mutation boundaries.
- `verification-evidence`: require component and hosted proof for authenticated envelope acceptance and fail-closed rejection paths.
- `verification-path-registry`: extend the hosted command ingress authority profile path to include authenticated envelope ordering and bounded claims.

## Impact

- Affected code:
  - `OBC/Components/CommandIngressAuthority`
  - hosted runtime/config parsing and topology state
  - command envelope helper/tests
  - repo-owned hosted command probe scripts
  - canonical verification and roadmap/architecture docs
- Public/runtime-visible impact:
  - command envelope v1 wire contract grows auth fields and MAC
  - hosted runtime/profile config gains repo-controlled auth config inputs
  - `CommandIngressAuthority` exposes dedicated auth failure evidence
- Non-goals:
  - no signature support
  - no persistent secure storage
  - no external key catalog loading
  - no full replay-protection claim
  - no file/unknown uplink authority
  - no simultaneous dual-link proof
  - no Pi/RF auth proof
