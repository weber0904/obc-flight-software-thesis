## Context

The maintained `OBC/TopCcsds` baseline already has two comm-managed uplink
surfaces and distinct UHF runtime roles, but its command-security model is
still centered on command envelope v1 and wire-level `SESSION_OPEN(seq0)`.
That model is sufficient for the current bounded claim set, yet it does not
match the commercial authorization sequence the repository now wants to adopt:

- `ReqAuth(ServiceID)`
- `Challenge = moduleSerial || randomNonce`
- `GetSessionKey(ServiceID, Challenge)` through a secure server
- `Response = HMAC(sKey, AKnownMessage)`
- `Authenticated / NotAuthenticated`

The new design must preserve current F' component ownership boundaries,
preserve `CommController`'s UHF runtime semantics, and avoid collapsing
`uhf-backup` versus `uhf-primary-after-failover` into a new source identity.

## Design

### Component responsibilities

This change adds a new real component:

- `SecureLinkAuthorizer`

Its responsibilities are:

- consume handshake uplink packets routed from `FprimeRouter.unknownDataOut`
- parse and validate the handshake wire family
- generate and downlink `Challenge`
- verify `Response`
- maintain pending challenge and active auth state per `(ingressPort, serviceId)`
- emit typed auth grant/revoke notifications to `CommandIngressAuthority`
- clear state on timeout, reboot, and explicit runtime invalidation

It SHALL NOT:

- parse or dispatch normal command packets
- evaluate command authority policy
- own beacon suppress or packet quiet
- directly notify `CommController`

`CommandIngressAuthority` remains the post-auth secure command owner. For the
new path it SHALL:

- parse secure command v2 packets on APID `0x0000`
- verify MAC using the active `sKey` previously granted by
  `SecureLinkAuthorizer`
- enforce strict-monotonic `sequenceNumber`
- evaluate existing command authority rules
- dispatch accepted commands
- synthesize repo-internal opened-session runtime side effects when auth is
  granted
- notify `CommController` of open/activity/revoke through the existing runtime
  observer contract

This keeps the official component interaction layered like the commercial
reference:

- secure server -> ground helper
- satellite auth owner -> secure command gate
- secure command gate -> runtime policy owner

### Service identity and key model

The new wire protocol uses `ServiceID`, not `source_id` or `key_slot`.

- `ServiceID = 1`: S-band secure command service
- `ServiceID = 2`: UHF secure command service

`uhf-backup` and `uhf-primary-after-failover` share `ServiceID = 2`. Their
behavioral separation remains in runtime role policy, not in wire identity.

`key_slot` becomes implementation-only keystore mapping. Both OBC and
`security-server-sim` map `serviceId -> root key` locally.

The challenge-response derivation is:

- `moduleSerial = "FPOBCSAT00000001"` by default, provided from runtime config
- `challenge = moduleSerial[16] || randomNonce[15]`
- `AKnownMessage = 32 bytes of 0xFF`
- `sKey = HMAC-SHA256(mKey, "AUTH-SKEY-V1" || serviceId || challenge)`
- `response = HMAC-SHA256(sKey, AKnownMessage)`

`response` and the active `sKey` are full `32`-byte HMAC outputs.

### Handshake wire family

Handshake packets are formal CCSDS payloads on APID `0x00FE`.

The common header is:

- `magic = 0x0BC0A701`
- `version = 1`
- `messageType`
- `serviceId`
- `reserved = 0`

Message types are:

- `REQ_AUTH = 1`
- `CHALLENGE = 2`
- `RESPONSE = 3`
- `AUTH_STATUS = 4`

Payloads are:

- `REQ_AUTH { serviceId }`
- `CHALLENGE { serviceId, challenge[31] }`
- `RESPONSE { serviceId, response[32] }`
- `AUTH_STATUS { serviceId, statusCode }`

`SecureLinkAuthorizer` SHALL reject malformed packets, unknown services, and
unexpected packet sequencing before mutating auth state.

### Topology and routing

Handshake uplink is routed through the existing stock F' escape hatch:

- `ComCcsds.fprimeRouter.unknownDataOut -> SecureLinkAuthorizer`
- `OBCComCcsds.fprimeRouter.unknownDataOut -> SecureLinkAuthorizer`

`SecureLinkAuthorizer` receives both `Fw::Buffer` and `FrameContext`, consumes
or rejects the packet, and returns ownership to the matching router path.

Handshake downlink is added as a first-class CCSDS packet path. The design
must not piggyback on event/tlm queues. Each band gets a dedicated handshake
queue/input so `FW_PACKET_HAND` reaches the same framer/driver chain used by
the existing per-band packet queues.

Implementation note for this first repo integration:

- The current implementation carries a narrow `lib/fprime` submodule delta in:
  - `Svc/Subtopologies/ComCcsds/ComCcsds.fpp`
  - `Svc/Subtopologies/ComCcsds/ComCcsdsConfig/ComCcsdsConfig.fpp`
- That delta is limited to adding formal handshake queueing/plumbing needed by
  the maintained hosted `TopCcsds` path.
- The parent repo carries this delta as a pinned submodule commit through the
  forked `lib/fprime` URL in `.gitmodules`, not as a local-only dirty working
  tree.
- Reviewers and later PR descriptions SHALL call this out explicitly so the
  framework-local patch remains visible until it is either upstreamed or
  replaced by a repo-local topology variant.

Normal secure commands remain on command APID `0x0000` and continue to enter
`CommandIngressAuthority` through the existing `commandOut` path.

### Secure command v2

Secure command v2 is a parallel wire format with its own magic/version and no
`source_id`, `key_slot`, or `session_id`.

Header:

- `magic = 0x0BC0DE02`
- `version = 2`
- `flags = 0`
- `headerLength = fixed`
- `sequenceNumber`
- `innerLength`
- `macLength = 32`
- `reserved = 0`

Body:

- `innerCommand` as serialized `Fw::CmdPacket`
- `authTag = HMAC-SHA256(sKey, "CMD-V2" || sequenceNumber || innerCommand)`

`CommandIngressAuthority` SHALL maintain secure-session state per
`(ingressPort, serviceId)`:

- `secureSessionActive`
- `activeServiceId`
- `activeSKey`
- `lastAcceptedSequence`
- `runtimeSessionToken`

### Session synthesis and runtime semantics

There is no wire-level `SESSION_OPEN` on the new path.

When `SecureLinkAuthorizer` verifies `RESPONSE` successfully, it SHALL notify
`CommandIngressAuthority` with `authGranted(ingressPort, serviceId, sKey)`.

`CommandIngressAuthority` SHALL then:

- mark the secure session active
- set `lastAcceptedSequence = 0`
- generate or update a local runtime session token for observability
- emit the same runtime opened-session side effects currently associated with
  accepted lifecycle open

The ground-side helper currently starts each newly authenticated secure session
at `sequenceNumber = 1`, but the protocol requirement is only that secure
commands become strict-monotonic after the first accepted command for that
session.

Accepted secure commands SHALL refresh the runtime activity observer and the
UHF suppress timer only when they belong to the currently active secure
session.

### Session expiry and invalidation

Auth/session state clears when:

- the runtime restarts
- `CommController` invalidates the band/role
- `180s` elapse without a newly accepted authenticated command

UHF role invalidation is especially important:

- `uhf-backup` auth state SHALL NOT survive promotion into
  `uhf-primary-after-failover`
- the role switch SHALL revoke the UHF secure session immediately
- the ground side must repeat `ReqAuth -> Challenge -> Response`

### Ground helper orchestration

The first implementation keeps stock GDS UI untouched and extends the
repository-owned helper/injection model.

The ground helper SHALL:

- monitor pass start / AOS timing outside OBC
- if no secure auth state is active for the selected band, periodically send
  `REQ_AUTH`
- read `CHALLENGE`
- call local `security-server-sim` for `GetSessionKey(serviceId, challenge)`
- compute `RESPONSE`
- repeat `RESPONSE` until `AUTH_STATUS = AUTHENTICATED`
- send a secure command v2 probe, initially `EPS_GET_STATUS(seq=1)`
- only then release any queued pass commands

### Validation boundary

This change requires:

- classic F' UT for the new `SecureLinkAuthorizer` component
- updated UT and helper tests for `CommandIngressAuthority`
- a repository-owned hosted secure-auth probe proving the exact active hosted
  path
- explicit evidence for UHF backup versus failover-primary separation

Non-goals remain:

- encryption
- RF proof
- secure boot or hardware key storage
- removal of legacy v1 path in this change
