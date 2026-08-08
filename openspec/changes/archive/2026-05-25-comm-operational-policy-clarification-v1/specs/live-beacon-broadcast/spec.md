## ADDED Requirements

### Requirement: Beacon Suppress Policy Stays Separate From Beacon Broadcast Proof

The live-beacon broadcast capability SHALL keep no-ACK beacon emission separate
from command-session suppression policy ownership.

#### Scenario: Broadcast proof does not imply suppress runtime closure
- **WHEN** the repository cites current live-beacon evidence
- **THEN** it SHALL describe that evidence as proof of bounded broadcast
  cadence, payload, capture, and decode only
- **AND** it SHALL NOT treat that proof as evidence that session-aware beacon
  suppression or resume runtime behavior is already implemented

### Requirement: Beacon Suppress And Resume Policy Is COMM-Owned

The current baseline SHALL treat UHF beacon suppress and resume as COMM policy
owned by `CommController`, with suppress intended to begin at accepted
`SESSION_OPEN(seq0)` and intended to resume after bounded inactivity timeout.

#### Scenario: Beacon policy owner stays outside BeaconPublisher
- **WHEN** reviewers inspect current beacon/session wording
- **THEN** `BeaconPublisher` SHALL remain a fixed-cadence broadcast producer
- **AND** it SHALL NOT be described as the owner of UHF command-session
  suppress or resume decisions
