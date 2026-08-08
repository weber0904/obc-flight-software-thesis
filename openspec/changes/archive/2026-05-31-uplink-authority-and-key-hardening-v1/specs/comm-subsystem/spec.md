## MODIFIED Requirements

### Requirement: Active CCSDS File Ingress Has Repo-Owned Sequence-Staging Governance

The active CCSDS file-uplink path SHALL be governed by a repo-owned owner before
packets reach `Svc::FileUplink`, and staged upload admission SHALL now depend
on both secure auth state and current runtime file-role policy.

#### Scenario: START packet requires valid path, active auth, and allowed role
- **WHEN** the active `ComCcsds` or `OBCComCcsds` file ingress receives a
  `Fw::FilePacket` `START`
- **THEN** the repo-owned file-ingress owner SHALL still reject absolute paths,
  `..`, non-allowlisted logical destinations, and physical symlink escape
- **AND** it SHALL also require active secure auth on that ingress/service
- **AND** it SHALL also require current runtime file-role policy to allow file
  upload on that ingress
- **AND** only then SHALL it rewrite the accepted logical
  `.sequence-staging/<leaf>` destination to the configured physical staging
  root before forwarding to `FileUplink`.

#### Scenario: UHF backup remains deny-only for staged upload
- **WHEN** UHF secure auth is active while runtime role remains `BACKUP`
- **THEN** staged file upload SHALL still be denied on that ingress
- **AND** accepted secure read/status continuity SHALL NOT by itself reopen
  staged upload authority.

#### Scenario: Explicit-switched UHF primary may upload after re-auth
- **WHEN** runtime role switches to `uhf-primary-after-failover`
- **AND** the prior UHF auth state has been revoked and a new valid UHF secure
  auth cycle completes
- **THEN** the active UHF CCSDS file path MAY admit `.sequence-staging/<leaf>`
  upload under the same bounded sequence-staging rules as S-band.

#### Scenario: Mid-transfer invalidation fails closed
- **WHEN** a staged transfer `START` was previously accepted
- **AND** secure auth times out, is revoked, or current runtime file-role
  policy later becomes deny
- **THEN** subsequent `DATA`, `END`, and `CANCEL` packets for that transfer
  SHALL be dropped until a new valid `START` is accepted after re-auth.

### Requirement: Comm-Managed Unknown Uplink Is Handshake-Only And Fail-Closed

The active comm-managed unknown uplink surface SHALL admit only handshake APID
`0x00FE` traffic owned by `SecureLinkAuthorizer` and SHALL reject any other
unknown uplink packet without mutating auth/session state.

#### Scenario: Malformed or unsupported handshake uplink is rejected without state mutation
- **WHEN** `SecureLinkAuthorizer` receives malformed handshake bytes, an
  unsupported `serviceId`, or an unexpected handshake message type on uplink
- **THEN** it SHALL reject and return the packet
- **AND** it SHALL leave pending challenge, active auth, and timeout state
  unchanged.

#### Scenario: Other unknown uplink families are not admitted
- **WHEN** any non-handshake unknown packet reaches the current comm-managed
  unknown uplink route
- **THEN** the packet SHALL be rejected and returned by the handshake owner
- **AND** the repository SHALL treat APID `0x00FE` handshake traffic as the
  only current admitted unknown-uplink family on that route.

### Requirement: File-Ingress Governance Claim Stays Bounded To Active Comm-Managed Paths

This change SHALL keep file-ingress governance claims bounded to the current
comm-managed S-band path and explicit-switched UHF failover-primary path.

#### Scenario: Docs do not over-claim generic file-uplink closure
- **WHEN** this change is documented or evidenced
- **THEN** it SHALL be valid to claim staged file-uplink closure for active
  S-band and re-authenticated `uhf-primary-after-failover`
- **AND** it SHALL still NOT claim generic arbitrary-file authority, backup UHF
  file admission, CFDP redesign, reliable-transfer redesign, or target proof.
