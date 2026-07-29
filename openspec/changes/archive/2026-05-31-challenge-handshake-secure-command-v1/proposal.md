## Why

The current command-security baseline proves source-bound `HMAC-SHA256`
envelope verification and bounded `SESSION_OPEN` freshness, but it does not
provide the challenge-response authorization flow needed for a more flight-like
secure command entry path. The repository also currently binds UHF runtime
session semantics to wire-level `SESSION_OPEN(seq0)`, which no longer matches
the intended commercial-style separation between secure authorization and
post-auth command transport.

This change introduces a governed secure authorization layer and a parallel
post-auth secure command path so the active hosted and target-compatible
baseline can authenticate by service/band first, then accept secure commands
without inventing a second ad hoc gateway policy or collapsing current UHF role
semantics.

## What Changes

- Add a new `SecureLinkAuthorizer` component that owns handshake packets on
  handshake APID `0x00FE`, generates `Challenge`, verifies `Response`, and
  maintains active auth state per comm-managed ingress/service.
- Add a new secure command v2 wire path on command APID `0x0000` that reuses
  `CommandIngressAuthority` as the post-auth gate for MAC verification,
  sequence enforcement, authority, dispatch, and runtime session observation.
- Replace the new path's wire-level dependency on `SESSION_OPEN` with
  `Authenticated` success that synthesizes the repo-internal opened-session
  state needed by `CommController` and the current runtime observer surfaces.
- Keep legacy command envelope v1 and `SESSION_OPEN(seq0)` behavior available
  during migration, but scope the new challenge-auth path as the formal secure
  command direction for comm-managed S-band and UHF surfaces.
- Add formal CCSDS routing and egress plumbing for handshake packets so
  `Challenge` and `AuthStatus` are first-class downlink packets rather than
  event/tlm side-band data.
- Add repository-owned hosted helpers and probes for
  `ReqAuth -> Challenge -> Response -> Authenticated -> secure command`
  validation on S-band and UHF, including UHF role invalidation and timeout
  behavior.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: add the secure authorization and secure command v2
  contract, scope `SESSION_OPEN` to legacy v1, and define auth-success session
  synthesis and secure-session timeout/revoke behavior.
- `comm-subsystem`: change the active UHF operational session boundary from
  accepted `SESSION_OPEN(seq0)` to accepted UHF authorization completion that
  synthesizes the runtime opened-session state, while keeping packet quiet and
  role semantics separate.
- `interface-contract-index`: require `docs/interfaces.md` to document the new
  handshake APID/service mapping, secure command v2 boundary, and the updated
  UHF suppress/runtime semantics.
- `verification-evidence`: require reviewable proof for the new handshake
  family, auth-success session synthesis, secure command v2 acceptance/reject
  behavior, and UHF backup/failover-primary role separation.

## Impact

- Affected code:
  - new `OBC/Components/SecureLinkAuthorizer`
  - `OBC/Components/CommandIngressAuthority`
  - `OBC/Components/CommController`
  - `OBC/TopCcsds` topology, per-band CCSDS routing, and handshake egress
    queueing
  - hosted helpers and new probe scripts under `scripts/`
- Public/operator impact:
  - new comm-managed secure authorization bootstrap flow for S-band and UHF
  - new handshake packet family on APID `0x00FE`
  - secure command v2 on APID `0x0000`
- Non-goals:
  - no encryption in this change
  - no RF behavior claim
  - no full replay protection claim beyond challenge-based session bootstrap
    plus per-session strict sequence
  - no immediate removal of legacy v1 envelope/`SESSION_OPEN`
