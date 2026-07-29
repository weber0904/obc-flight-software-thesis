## MODIFIED Requirements

### Requirement: Beacon Suppress Policy Stays Separate From Beacon Broadcast Proof

The live-beacon broadcast capability SHALL keep no-ACK beacon emission
separate from command-session suppress policy ownership even after suppress
runtime closure is implemented.

#### Scenario: Broadcast proof alone is not suppress proof
- **WHEN** the repository cites current live-beacon evidence without the
  dedicated suppress/runtime record
- **THEN** it SHALL describe that evidence as proof of bounded broadcast
  cadence, payload, capture, and decode only
- **AND** it SHALL NOT treat ordinary broadcast proof alone as evidence for
  suppress start, refresh, invalidation clear, or timeout resume behavior

### Requirement: Beacon Suppress And Resume Policy Is COMM-Owned

The current baseline SHALL treat UHF beacon suppress and resume as COMM runtime
owned by `CommController`, with suppress beginning at accepted authenticated
UHF `SESSION_OPEN(seq0)`, refresh extending only on accepted authenticated UHF
same-session activity, and resume happening after immediate session
invalidation or fixed bounded inactivity timeout.

#### Scenario: Beacon policy owner stays outside BeaconPublisher
- **WHEN** reviewers inspect current beacon/session runtime wording
- **THEN** `BeaconPublisher` SHALL remain a fixed-cadence broadcast producer
- **AND** it SHALL NOT be described as the owner of UHF command-session
  suppress or resume decisions
