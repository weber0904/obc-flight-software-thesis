## Why

The current secure-auth, secure-command, and bounded uplink-authority behavior
is proven on hosted paths only. The active target COMM baseline now includes
repo-owned node-`5` S-band and physical node-`6` UHF proof paths, but the
new challenge-auth secure command path and keystore-backed file/unknown-uplink
authority closure have not yet been exercised on those target paths.

Without a target-owned proof, the repository cannot claim that the installed
release, systemd working directory, bundled keystore, gateway captures, and
target journal truth preserve the hosted secure-auth behavior on the lab
hardware topology.

## What Changes

- Add a governed `target-secure-auth-proof-v1` OpenSpec change.
- Add a repository-owned target proof entrypoint,
  `scripts/run_target_secure_auth_proof.sh`.
- Extend the existing target CAN matrix harness with a `secure-auth-proof`
  mode rather than inventing a new target workflow.
- Prove target S-band secure auth over registry entry `59`:
  APID `0x00FE` challenge auth, secure command v2 on command APID, non-`1`
  first secure-command sequence, strict next-sequence behavior, malformed
  handshake fail-closed behavior, installed-release keystore provenance, and
  `.sequence-staging/<leaf>` staged upload admission after secure auth.
- Prove bounded target UHF secure-auth behavior over registry entry `69`:
  `ServiceID = 2` on physical node `6`, `uhf-backup` read/status acceptance,
  `uhf-backup` high-authority denial, `uhf-backup` staged-upload denial, role
  switch invalidating old auth/session state, and re-auth before
  `uhf-primary-after-failover` secure-command acceptance.
- Update evidence, registry, interface, and operator documentation to record
  the target proof boundary and remaining non-claims.

## Capabilities

### New Capabilities

- None. This change is verification-first and should make product changes only
  if target execution exposes a real bug.

### Modified Capabilities

- `comm-subsystem`: record the target proof boundary for secure auth,
  secure-command v2, bounded staged-file admission, UHF backup denial, and
  UHF role-switch re-auth.
- `verification-evidence`: require target evidence artifacts for the installed
  release provenance and S-band/UHF proof cases.
- `verification-path-registry`: add a target secure-auth proof entry without
  collapsing hosted entries `43E/43F`, target S-band entry `59`, or dual-link
  entry `69`.
- `interface-contract-index`: update the secure-auth observability and target
  status wording after target evidence exists.

## Impact

- Affected code:
  - `scripts/run_target_secure_auth_proof.sh`
  - `scripts/comm_verification/lib/run_target_can_matrix_probe.py`
  - target proof evidence and operator docs
- Public/runtime-visible impact:
  - none expected unless the target proof exposes a product defect
- Non-goals:
  - no encryption
  - no RF proof
  - no boot-trust expansion
  - no hardware-backed or persistent secure key storage
  - no generic file authority beyond `.sequence-staging/<leaf>`
  - no UHF primary staged-upload success claim
  - no one-GDS aggregation or one-gateway multiplexing claim
  - no legacy command envelope v1 retirement
