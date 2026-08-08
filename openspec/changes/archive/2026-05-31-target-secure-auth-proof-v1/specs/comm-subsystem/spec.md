## ADDED Requirements

### Requirement: Target Secure Auth Proof Reuses Existing S-Band And UHF Paths

The target secure-auth proof SHALL reuse the repository-owned target S-band
node-`5` path and physical node-`6` UHF path instead of defining a new target
transport or launch workflow.

#### Scenario: S-band target proof stays on registry entry 59
- **WHEN** `target-secure-auth-proof-v1` proves S-band secure auth behavior
- **THEN** it SHALL run through the existing macOS `fprime-gds` plus
  `ground_ttc_gateway` to `subsystem.local` node `5`, SocketCAN, and
  `obc.local` path
- **AND** it SHALL record that hosted secure-auth evidence is ancestry only,
  not target proof.

#### Scenario: UHF target proof stays bounded on registry entry 69
- **WHEN** `target-secure-auth-proof-v1` proves UHF secure auth behavior
- **THEN** it SHALL use the existing physical node-`6` `uhf-backup` adjunct
  plus explicit switch to `uhf-primary-after-failover`
- **AND** it SHALL keep `uhf-backup` and `uhf-primary-after-failover`
  authority claims separate.

### Requirement: Target Secure Auth Proof Keeps Runtime Scope Bounded

The target secure-auth proof SHALL validate target deployment behavior for the
already-designed secure-auth and bounded uplink-authority flows without adding
new feature scope unless the target proof exposes a product defect.

#### Scenario: S-band target proof covers secure command and staged upload
- **WHEN** the target S-band proof completes
- **THEN** it SHALL show APID `0x00FE` challenge auth, secure command v2 on
  the command APID, non-`1` first accepted secure-command sequence, strict
  next-sequence enforcement, malformed handshake fail-closed behavior, and
  `.sequence-staging/<leaf>` upload admission after secure auth.

#### Scenario: UHF target proof covers backup denial and re-auth
- **WHEN** the target UHF proof completes
- **THEN** it SHALL show `ServiceID = 2` auth on physical node `6`,
  `uhf-backup` read/status secure-command acceptance, `uhf-backup`
  high-authority denial, `uhf-backup` staged-upload denial, old auth/session
  invalidation after switch, and re-auth before
  `uhf-primary-after-failover` secure-command acceptance
- **AND** it SHALL NOT claim UHF primary staged-upload success in this change.

#### Scenario: Non-goals remain explicit
- **WHEN** the target secure-auth proof is documented
- **THEN** it SHALL NOT claim encryption, RF closure, boot-trust expansion,
  hardware-backed key storage, generic file authority, one-GDS aggregation,
  one-gateway multiplexing, or legacy v1 retirement.
