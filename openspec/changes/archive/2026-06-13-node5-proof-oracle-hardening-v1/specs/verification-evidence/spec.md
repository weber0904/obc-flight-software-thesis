## ADDED Requirements

### Requirement: Node-5 Observability Proof Oracles Stay Packet-Path Grounded
Maintained hosted and target node-`5` observability-governance evidence SHALL
prove representative bounded detailed readback and S-band close on the
packetized path itself, not only through passive observer quiescence.

#### Scenario: Hosted and target bounded detailed readback stays bounded on the packet path
- **WHEN** hosted or target node-`5` observability evidence records the
  representative authenticated `EPS_GET_STATUS -> EPS_IBAT` detailed readback
- **THEN** the evidence SHALL show one auditable detailed readback on the
  maintained path
- **AND** it SHALL keep that detailed readback tied to a bounded
  command-specific capture or readback artifact on the maintained packet path
  instead of relying only on a long-running passive channel listener.

#### Scenario: Hosted and target switch-close quiet stays packet-path grounded
- **WHEN** the same hosted or target observability evidence records close on a
  primary switch away from S-band
- **THEN** the evidence SHALL show that the maintained downlink or gateway
  capture stops growing after the close condition
- **AND** it SHALL not accept passive-listener silence by itself as sufficient
  proof of packet-path quiet.

### Requirement: Target Secure-Auth Handshake Recovery May Use Source-Aware Progress
Maintained target secure-auth evidence SHALL allow handshake observation to
advance on the governed wire capture or native packet-log surface, provided the
evidence still records which source confirmed each step and preserves the same
target path identity.

#### Scenario: Evidence records source-aware handshake confirmation
- **WHEN** target secure-auth evidence records a challenge or auth-status step
  on a maintained path
- **THEN** it SHALL record whether the confirming surface was wire capture,
  native packet log, or target journal
- **AND** it SHALL keep the result tied to the same target secure-auth path
  rather than inventing a parallel proof family.
