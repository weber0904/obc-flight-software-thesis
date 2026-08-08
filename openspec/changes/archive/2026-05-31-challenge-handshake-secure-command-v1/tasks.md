## 1. OpenSpec Artifacts

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `core-system-contracts`, `comm-subsystem`, `interface-contract-index`, and `verification-evidence`.
- [x] 1.2 Validate the change artifacts before implementation with `openspec validate challenge-handshake-secure-command-v1`.

## 2. SecureLinkAuthorizer Component

- [x] 2.1 Add `OBC/Components/SecureLinkAuthorizer` as a real F' component with formal ports for handshake uplink, handshake downlink, auth grant/revoke signaling, scheduler-driven timeout handling, and runtime invalidation.
- [x] 2.2 Implement handshake packet parsing/encoding, challenge generation, response verification, and active auth-state tracking per `(ingressPort, serviceId)`.
- [x] 2.3 Add a classic F' unit-test harness for `SecureLinkAuthorizer` plus helper-level tests for the handshake wire helpers and HMAC derivation logic.

## 3. Secure Command V2 Gate

- [x] 3.1 Extend `CommandIngressAuthority` to accept auth grant/revoke signals from `SecureLinkAuthorizer`, synthesize repo-internal opened-session state, and expose the existing runtime observer side effects without wire-level `SESSION_OPEN`.
- [x] 3.2 Add secure command v2 parsing and HMAC verification while keeping legacy v1 envelope/`SESSION_OPEN` behavior intact in parallel.
- [x] 3.3 Update `CommandIngressAuthority` tests to cover auth-granted session synthesis, secure command v2 acceptance/reject behavior, timeout/revoke handling, and legacy non-regression.

## 4. Topology And Routing

- [x] 4.1 Wire S-band and UHF `FprimeRouter.unknownDataOut` to `SecureLinkAuthorizer` and return consumed buffers to the matching router paths.
- [x] 4.2 Add formal handshake downlink queueing/plumbing on both bands so `FW_PACKET_HAND` packets travel through the governed CCSDS downlink path instead of event/tlm side-band output.
- [x] 4.3 Connect runtime invalidation signals from `CommController` into `SecureLinkAuthorizer` and preserve existing command/file routing unchanged.

## 5. Ground Helpers, Probes, And Evidence

- [x] 5.1 Add or extend repository-owned ground helper scripts for `REQ_AUTH -> CHALLENGE -> RESPONSE -> AUTHENTICATED -> secure command probe` on the hosted S-band and UHF paths using the existing TTS injection model.
- [x] 5.2 Add a hosted secure-auth probe proving S-band success, UHF backup read/status continuity, UHF failover-primary re-auth behavior, timeout clearing, and legacy v1 non-regression.
- [x] 5.3 Add bounded evidence under `evidence/records/` and update `evidence/verification-path-registry.md` for the new hosted secure-auth validation path.

## 6. Docs And Validation

- [x] 6.1 Update `docs/interfaces.md` and `docs/architecture/current-development-architecture.md` so the current baseline reflects the handshake APID, service IDs, secure command v2, and auth-success UHF session boundary.
- [x] 6.2 Update `docs/roadmap/next-work.md` to retire the nonce/challenge gap for the active baseline while leaving encryption and hardware-backed security as future work.
- [x] 6.3 Run focused UT, hosted probes, `openspec validate challenge-handshake-secure-command-v1`, and `openspec validate --specs`.
