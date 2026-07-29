## ADDED Requirements

### Requirement: UHF Primary Packet Quiet Evidence Is Distinct From Beacon Suppress Evidence

The verification evidence set SHALL distinguish UHF primary packet quiet truth
from UHF beacon suppress/runtime truth, SHALL separately record preservation of
official file/data-product downlink during UHF primary packet quiet, and SHALL
keep current non-claims explicit.

#### Scenario: Hosted evidence proves packet quiet before accepted session open
- **WHEN** the repository records fresh hosted evidence for
  `uhf-primary-after-failover`
- **THEN** that evidence SHALL show that live `event/tlm` packet egress is
  already suppressed before any accepted qualifying UHF `SESSION_OPEN(seq0)`
- **AND** it SHALL separately show that beacon remains visible until accepted
  qualifying session ownership starts suppress

#### Scenario: Hosted evidence preserves beacon suppress truth as a separate claim
- **WHEN** the repository records fresh hosted evidence for the same UHF
  primary runtime surface
- **THEN** it SHALL separately show that accepted qualifying UHF
  `SESSION_OPEN(seq0)` still starts beacon suppress and same-session accepted
  UHF activity still refreshes the bounded active window
- **AND** it SHALL not collapse packet quiet and beacon suppress into one
  undifferentiated runtime claim

#### Scenario: Evidence records formal file/downlink preservation and non-claims
- **WHEN** the repository records fresh hosted packet-quiet evidence
- **THEN** it SHALL show that official file/data-product downlink remains
  available while UHF primary packet quiet is active
- **AND** it SHALL keep explicit non-claims for simultaneous dual-link
  runtime, UHF reliable transfer, RF closure, and broader target non-quiet
  closure
