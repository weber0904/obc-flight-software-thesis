## Why

The active hosted secure-auth baseline now proves `ReqAuth -> Challenge ->
Response -> Authenticated` plus secure command v2, but the repository still has
three post-handshake security gaps:

- comm-managed unknown uplink is only incidentally fail-closed through the
  current handshake implementation and is not yet a formal authority claim
- the active `.sequence-staging/<leaf>` file-uplink path is governed only by
  path-rewrite policy, not by secure auth plus current band/role authority
- comm-managed auth key material is still sourced through runtime injection and
  duplicated across OBC and helper tooling rather than coming from one tracked
  keystore contract

This change closes those gaps on the hosted baseline without broadening into
encryption, target proof, generic file-uplink governance, or legacy retirement.

## What Changes

- Add one tracked command-auth keystore asset and load both secure-auth root
  keys and retained comm-managed legacy v1 keys from that shared contract.
- Remove hosted runtime support for `--command-auth*` and `COMMAND_AUTH_*`
  injection surfaces.
- Formalize comm-managed unknown uplink as a handshake-only fail-closed surface
  on APID `0x00FE`, with explicit reject behavior for malformed or unsupported
  packets.
- Extend `FileIngressAuthority` so `.sequence-staging/<leaf>` admission
  requires both active secure auth and current comm-managed file-role allow
  state.
- Add typed runtime file-policy signaling from `CommController` into
  `FileIngressAuthority`, and bind secure-auth activity/revoke semantics to
  staged file upload.
- Keep legacy v1 present only as a narrowed compatibility path backed by the
  same keystore asset.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `core-system-contracts`: extend the current bounded authority story to
  include comm-managed handshake-only unknown uplink, staged file-uplink
  closure, and repo-tracked keystore-backed auth material.
- `comm-subsystem`: refine the active file-ingress claim so sequence-staging
  admission depends on secure auth and runtime role policy, and refine
  unknown-uplink truth to handshake-only fail-closed behavior.
- `interface-contract-index`: require `docs/interfaces.md` to record the new
  keystore asset contract, handshake-only unknown uplink, and staged
  file-uplink authority boundary.
- `verification-evidence`: require reviewable hosted proof for keystore-backed
  secure auth, malformed handshake rejection, staged file admission/denial by
  role, and legacy compatibility using the tracked keystore.

## Impact

- Affected code:
  - `OBC/Runtime/HostedRuntime`
  - `OBC/Main.cpp` and `OBC/TopCcsds/OBCAppTopology.cpp`
  - `OBC/Components/SecureLinkAuthorizer`
  - `OBC/Components/FileIngressAuthority`
  - `OBC/Components/CommController`
  - repo-owned helper and probe scripts under `scripts/`
- Affected docs/specs:
  - `docs/interfaces.md`
  - `docs/architecture/current-development-architecture.md`
  - `docs/verification-path-registry.md`
  - `openspec/specs/core-system-contracts/spec.md`
  - `openspec/specs/comm-subsystem/spec.md`
  - `openspec/specs/interface-contract-index/spec.md`
  - `openspec/specs/verification-evidence/spec.md`

## Non-Claims

This change does not claim:

- target or lab secure-auth proof
- encryption
- persistent secure key storage or hardware-backed key storage
- full generic file-uplink governance beyond `.sequence-staging/<leaf>`
- retirement of the retained legacy v1 command envelope path
- boot/trust-chain hardening
