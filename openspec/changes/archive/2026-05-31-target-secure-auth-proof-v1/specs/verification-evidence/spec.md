## ADDED Requirements

### Requirement: Target Secure Auth Evidence Is Reviewable

Verification evidence for `target-secure-auth-proof-v1` SHALL record the target
commands, proof root, provenance gates, gateway captures, target journal
snapshots, checkpoint stream, summary JSON, cleanup status, and final verdict.

#### Scenario: Evidence records installed release provenance
- **WHEN** target secure-auth evidence is recorded
- **THEN** it SHALL include the installed release `current` symlink, service
  `WorkingDirectory`, bundled `config/security/command-auth.ini` SHA,
  manifest SHA, expected target and subsystem service identities, and absence
  of forbidden `COMMAND_AUTH_*` service environment injection and
  `--command-auth*` CLI injection
- **AND** it SHALL state whether the existing installed release was accepted or
  whether sync/bootstrap/package/install was rerun.

#### Scenario: Evidence records S-band target proof cases
- **WHEN** target S-band secure-auth evidence is recorded
- **THEN** it SHALL include APID `0x00FE` challenge auth, secure command v2 on
  the command APID, non-`1` first accepted sequence, strict next-sequence
  rejection, malformed handshake fail-closed behavior, and
  `.sequence-staging/<leaf>` upload admission after secure auth.

#### Scenario: Evidence records bounded UHF target proof cases
- **WHEN** target UHF secure-auth evidence is recorded
- **THEN** it SHALL include physical node-`6` `ServiceID = 2` auth,
  `uhf-backup` read/status acceptance, `uhf-backup` high-authority denial,
  `uhf-backup` staged-upload denial, switch invalidation of old UHF
  auth/session state, and re-auth before `uhf-primary-after-failover`
  secure-command acceptance
- **AND** it SHALL state that UHF primary staged-upload success remains outside
  this change.

#### Scenario: Failed target proof is classified before product changes
- **WHEN** the target proof fails
- **THEN** the evidence SHALL classify the failure first as provenance,
  environment, or probe-oracle failure before treating it as a product defect.
