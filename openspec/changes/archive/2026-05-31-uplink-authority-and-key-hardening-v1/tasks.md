## 1. OpenSpec And Shared Config

- [x] 1.1 Write `proposal.md`, `design.md`, `tasks.md`, and delta specs for `core-system-contracts`, `comm-subsystem`, `interface-contract-index`, and `verification-evidence`.
- [x] 1.2 Add the tracked keystore asset and shared parser/helper surfaces used by OBC and repo-owned Python tooling.
- [x] 1.3 Validate the change artifacts before implementation with `openspec validate uplink-authority-and-key-hardening-v1`.

## 2. Keystore-Backed Auth Configuration

- [x] 2.1 Update hosted OBC startup/config paths so secure auth and retained comm-managed legacy v1 auth load from the tracked keystore instead of runtime-injected key settings.
- [x] 2.2 Remove hosted runtime support for `--command-auth*` and migrate affected runtime/help text accordingly.
- [x] 2.3 Update `security-server-sim` and repo-owned helper/probe tooling to read the shared keystore asset instead of `COMMAND_AUTH_*` or equivalent CLI root-key injection.

## 3. Unknown Uplink Authority Closure

- [x] 3.1 Formalize handshake-only APID `0x00FE` reject behavior in `SecureLinkAuthorizer` with explicit non-mutation handling for malformed or unsupported unknown uplink.
- [x] 3.2 Add or refresh focused `SecureLinkAuthorizer` tests covering valid keystore-backed handshake success and malformed/unsupported unknown uplink rejection.

## 4. File Ingress Authority Closure

- [x] 4.1 Add typed runtime file-policy signaling from `CommController` to `FileIngressAuthority`.
- [x] 4.2 Extend `FileIngressAuthority` so `.sequence-staging/<leaf>` START admission requires both active secure auth and allowed runtime file role.
- [x] 4.3 Bind file-ingress activity/revoke semantics to the secure-auth inactivity and invalidation model, including fail-closed mid-transfer behavior.
- [x] 4.4 Add or refresh `FileIngressAuthority` tests covering S-band allow, UHF backup deny, failover-primary allow-after-reauth, mid-transfer revoke/timeout drop, and non-staging destination rejection.

## 5. Hosted Proof, Docs, And Governance

- [x] 5.1 Update or add a hosted probe proving S-band staged upload success, UHF backup deny, UHF failover-primary re-auth upload success, malformed handshake rejection, and retained legacy v1 compatibility through the tracked keystore.
- [x] 5.2 Update `docs/interfaces.md`, `docs/architecture/current-development-architecture.md`, and `evidence/verification-path-registry.md` to reflect keystore-backed auth, handshake-only unknown uplink, and staged file-uplink closure.
- [x] 5.3 Update repo-owned operator/helper docs so maintained workflows no longer rely on `COMMAND_AUTH_*` or `--command-auth-*`.

## 6. Validation

- [x] 6.1 Run focused UT for `SecureLinkAuthorizer` and `FileIngressAuthority` plus any keystore helper tests.
- [x] 6.2 Run the hosted proof for this change.
- [x] 6.3 Run `openspec validate uplink-authority-and-key-hardening-v1` and `openspec validate --specs`.
