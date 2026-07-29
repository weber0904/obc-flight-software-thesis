## ADDED Requirements

### Requirement: UHF Primary Live Packet Egress Is Non-Quiet In The Current Baseline

The current COMM runtime SHALL keep live `event/tlm` packet egress enabled when
UHF is the current primary band, unless a separate diagnostic quiet override is
explicitly enabled.

#### Scenario: UHF primary no longer suppresses live packets by default
- **WHEN** the runtime promotes UHF to the current primary band through
  explicit operator policy or executor-owned fault recovery
- **THEN** it SHALL continue to route live packet egress on the current UHF
  primary path
- **AND** it SHALL NOT require a packet-quiet disable override to make bounded
  live readback observable.

#### Scenario: Diagnostic quiet remains separate from UHF primary semantics
- **WHEN** a bounded proof or diagnosis enables the dedicated diagnostic quiet
  overlay
- **THEN** the runtime MAY still suppress packet egress on both bands for that
  probe-owned diagnostic purpose
- **AND** that overlay SHALL NOT redefine the maintained non-quiet UHF primary
  product truth.

### Requirement: Current Autonomous Failover Truth Uses Detector Plus Executor

The current maintained `S-band -> UHF` failover truth SHALL be
`COMM_PRIMARY_UNAVAILABLE` detection plus executor-owned COMM failover rather
than a current proof family centered on manual `COMM_SET_ACTIVE(UHF)`.

#### Scenario: Autonomous failover promotes UHF after current-primary loss
- **WHEN** the current primary S-band COMM stand-in becomes unavailable for the
  configured detector threshold
- **THEN** `CommController` SHALL latch `COMM_PRIMARY_UNAVAILABLE`
- **AND** `RecoveryExecutor` SHALL own the bounded failover actuation that
  promotes UHF to the current primary link set.

#### Scenario: Post-failover UHF command truth requires fresh auth
- **WHEN** executor-owned COMM recovery promotes UHF to primary
- **THEN** the runtime SHALL invalidate stale UHF backup auth/session state
- **AND** it SHALL require a fresh accepted UHF secure-auth cycle before
  bounded UHF primary secure-command readback succeeds again.

## REMOVED Requirements

### Requirement: UHF Primary Packet Quiet Suppresses Live Packet Noise But Preserves Official File Downlink
**Reason**: UHF primary packet quiet is no longer the maintained product truth.
Live packet egress remains enabled on the current UHF primary path, while
beacon suppress stays as a separate auth-triggered mechanism.
**Migration**: Use `UHF Primary Live Packet Egress Is Non-Quiet In The Current Baseline`.

### Requirement: Future Target-Bearing Dual-Link Claim Stays Primary-Led And Switch-Closed
**Reason**: The maintained current failover truth now relies on detector-driven
loss and executor-owned autonomous promotion rather than explicit-switch-only
current closure.
**Migration**: Use `Current Autonomous Failover Truth Uses Detector Plus Executor`
for the maintained target path and preserve explicit-switch proofs as
historical evidence only.
