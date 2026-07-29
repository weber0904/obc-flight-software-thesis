## ADDED Requirements

### Requirement: Hosted UHF Primary Packet Quiet Path Is Registered Separately

The verification-path registry SHALL include a distinct hosted UHF primary
packet quiet entry separate from the hosted UHF beacon suppress/runtime entry
once fresh governed evidence proves the decoupled semantics.

#### Scenario: Registry names the hosted packet-quiet boundary separately
- **WHEN** hosted UHF primary packet-quiet evidence passes
- **THEN** the registry SHALL identify a distinct hosted path for
  `uhf-primary-after-failover` packet quiet before accepted qualifying
  `SESSION_OPEN(seq0)`
- **AND** it SHALL keep the hosted beacon suppress/runtime path as a separate
  adjacent entry rather than merging the two

#### Scenario: Registry states packet quiet scope without widening baseline claims
- **WHEN** reviewers inspect the hosted UHF primary packet-quiet registry entry
- **THEN** it SHALL say that live `event/tlm` packet egress is suppressed
  whenever UHF is the current primary band and that official file/data-product
  downlink remains formal
- **AND** it SHALL keep explicit non-claims for simultaneous dual-link
  runtime, UHF reliable transfer, RF closure, and target non-quiet closure
