## Context

The current active hosted baseline already routes:

- comm-managed unknown uplink to `SecureLinkAuthorizer`
- active CCSDS file upload through `FileIngressAuthority`
- secure command v2 through `CommandIngressAuthority`

That leaves the remaining gaps narrowly scoped:

- unknown uplink reject behavior exists in code but is not yet a formal
  handshake-only authority claim
- `.sequence-staging/<leaf>` upload is governed only by logical-path policy and
  does not yet require secure auth plus band/role authority
- root keys are still injected through hosted runtime and helper script
  arguments instead of one tracked keystore contract

The design must keep secure command v2 as the active direction, keep legacy v1
available only for compatibility, and remain hosted-scoped.

## Goals

- replace comm-managed runtime key injection with one tracked keystore asset
- turn unknown uplink into a formal handshake-only fail-closed surface
- make staged file ingress depend on active secure auth and runtime file-role
  policy
- keep UHF backup versus failover-primary file authority explicit
- migrate maintained helpers and probes away from `COMMAND_AUTH_*` and
  `--command-auth-*`

## Non-Goals

- target secure-auth proof
- encryption
- hardware-backed or persistent secure key storage
- generic arbitrary file-uplink governance
- reliable-transfer redesign
- legacy v1 retirement

## Design Decisions

### 1. One repo-tracked keystore asset owns comm-managed auth defaults

Add a tracked asset at `config/security/command-auth.ini` with this contract:

```ini
module_serial=FPOBCSAT00000001

[sband]
source_id=1
key_slot=1
key_hex=<64 hex chars>

[uhf]
source_id=2
key_slot=2
key_hex=<64 hex chars>
```

The file is a repo-controlled dev/default keystore, not a persistent secure
store and not a target-hardware secret provisioning mechanism.

Both OBC and repo-owned helper tooling read this same file:

- `SecureLinkAuthorizer` maps `serviceId=1` to `[sband]` and `serviceId=2` to
  `[uhf]`
- retained comm-managed legacy v1 auth uses the corresponding `source_id`,
  `key_slot`, and `key_hex`
- `security-server-sim` reads the same file to derive `sKey`

### 2. Hosted runtime no longer accepts command-auth injection

Hosted runtime support for:

- `--command-auth`
- `--command-auth-source-id`
- `--command-auth-key-slot`
- `--command-auth-key-hex`

is removed outright.

The hosted OBC startup path now always derives comm-managed auth defaults from
the tracked keystore asset. Repo-owned launchers and probes must stop setting
`COMMAND_AUTH_*`.

### 3. Unknown uplink is handshake-only and fail-closed

The active comm-managed unknown uplink surface remains:

- `FprimeRouter.unknownDataOut -> SecureLinkAuthorizer`

Only handshake APID `0x00FE` traffic is admitted on that surface. Everything
else is rejected, returned, and must not mutate pending challenge, active auth,
or timeout state.

Reject classes include:

- malformed CCSDS payload
- bad handshake magic/version
- unsupported `serviceId`
- unexpected handshake message type on uplink
- any non-handshake unknown packet family

This change turns that behavior into a claimed contract with counters/events
and focused proof.

### 4. File ingress now depends on both secure auth and runtime role policy

`FileIngressAuthority` keeps the existing narrow logical destination family:

- `.sequence-staging/<leaf>`

but START admission now requires:

1. valid logical destination under the current governed prefix
2. active secure auth state on that ingress/service
3. file-role allow state from `CommController`

V1 file-role policy is:

- allow `sband-primary`
- allow explicit-switched `uhf-primary-after-failover`
- deny `uhf-backup`

All other destinations remain reject-only.

### 5. CommController owns runtime file policy; FileIngressAuthority enforces it

Add a small typed policy message from `CommController` to
`FileIngressAuthority` carrying per-ingress runtime file authority:

- ingress port
- link identity
- link role
- fileAllowed bool

`CommController` remains the owner of identity/role truth. `FileIngressAuthority`
does not infer backup or primary from packet contents.

### 6. Secure auth activity and revoke semantics extend to staged file upload

`FileIngressAuthority` integrates with the secure-auth protocol surface:

- active auth grant marks file ingress eligible once role also allows it
- accepted file packets emit secure-auth activity refresh
- revoke, timeout, or role invalidation immediately clears staged-upload
  authority

Mid-transfer behavior is fail-closed:

- if `START` was previously accepted but auth or role later becomes invalid,
  later `DATA` / `END` / `CANCEL` packets are dropped
- a new valid `START` after re-auth is required before transfer may resume

### 7. Legacy v1 stays compatible but does not gain new semantics

Legacy command envelope v1 remains present for comm-managed compatibility and
reads the same tracked keystore asset. This change does not broaden legacy v1
behavior and does not preserve authenticated `dev-direct` or `internal` as
first-class current-baseline profiles.

### 8. Validation remains hosted and bounded

This change requires:

- helper/L1 tests for the keystore parser
- `SecureLinkAuthorizer` UT proving keystore-backed handshake success plus
  malformed/unsupported unknown uplink reject-without-mutation behavior
- `FileIngressAuthority` UT proving secure-auth/role-gated START behavior and
  mid-transfer fail-closed revoke
- hosted probe proving:
  - S-band secure auth + staged upload + official sequence admission success
  - UHF backup secure auth + staged upload denial
  - explicit UHF failover-primary re-auth + staged upload success
  - bad APID `0x00FE` or malformed handshake reject without auth state creation
  - retained legacy v1 compatibility through the shared keystore
