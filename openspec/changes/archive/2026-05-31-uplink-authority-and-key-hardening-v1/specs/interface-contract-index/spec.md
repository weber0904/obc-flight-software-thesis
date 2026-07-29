## MODIFIED Requirements

### Requirement: Interface Index Covers Current Baseline Boundaries

The interface contract index SHALL record the hosted-baseline keystore-backed
auth contract, handshake-only unknown uplink, and staged file-uplink authority
boundary in addition to the existing secure-command and UHF runtime facts.

#### Scenario: Reviewers can audit the tracked keystore contract in one place
- **WHEN** a reviewer inspects the command-security or COMM sections of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that comm-managed auth defaults come from
  one tracked keystore asset
- **AND** they SHALL be able to see that the asset carries `module_serial`,
  S-band legacy `source_id/key_slot/key`, and UHF legacy `source_id/key_slot/key`
  entries
- **AND** they SHALL be able to see that old hosted runtime
  `--command-auth-*` injection is no longer part of the active baseline.

#### Scenario: Reviewers can audit unknown uplink and staged file authority separately
- **WHEN** a reviewer inspects the file-ingress and unknown-uplink sections of
  `docs/interfaces.md`
- **THEN** they SHALL be able to see that APID `0x00FE` handshake traffic is
  the only admitted current unknown-uplink family on the comm-managed route
- **AND** they SHALL be able to see that `.sequence-staging/<leaf>` upload now
  requires both active secure auth and current allowed runtime file role
- **AND** they SHALL be able to see that `uhf-backup` remains deny-only for
  staged upload while explicit-switched `uhf-primary-after-failover` may admit
  staged upload only after re-auth.
