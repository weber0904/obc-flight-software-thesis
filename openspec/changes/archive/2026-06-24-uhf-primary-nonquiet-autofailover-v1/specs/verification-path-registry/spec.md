## ADDED Requirements

### Requirement: Current Registry Distinguishes Historical Explicit-Switch UHF Paths From Maintained Autonomous Failover Paths

The verification-path registry SHALL keep older quiet or explicit-switch UHF
primary records reviewable as historical evidence while registering maintained
current route closure only on the non-quiet autonomous-failover path.

#### Scenario: Historical explicit-switch records remain visible but not current
- **WHEN** the registry cites older target UHF primary or dual-link records
- **THEN** it SHALL mark those records as historical or superseded if their
  acceptance boundary depended on explicit `COMM_SET_ACTIVE(UHF)` or on UHF
  primary packet quiet
- **AND** it SHALL NOT cite them as the maintained Chapter 5 current closure
  path.

#### Scenario: Current autonomous failover path is registered distinctly
- **WHEN** the new autonomous failover proof passes
- **THEN** the registry SHALL name the maintained target path as
  detector-triggered `COMM_PRIMARY_UNAVAILABLE` plus executor-owned promotion
  to non-quiet `uhf-primary-after-failover`
- **AND** it SHALL identify the governing evidence for post-failover UHF
  re-auth and bounded readback separately from historical explicit-switch
  proofs.
