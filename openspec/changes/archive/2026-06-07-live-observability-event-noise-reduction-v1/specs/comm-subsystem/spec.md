## ADDED Requirements

### Requirement: Routine Healthy CSP Ping Success Is Not Part Of The Current Ground Event Surface

The current comm baseline SHALL keep periodic CSP ping success out of the
default packetized operator event stream while preserving failure and link
transition visibility.

#### Scenario: Healthy ping success updates counters without a success event
- **WHEN** `CspBridge` completes a ping command or runtime health probe and
  the runtime call succeeds with `success = true`
- **THEN** it SHALL continue updating the existing CSP counters and command
  response semantics
- **AND** it SHALL NOT emit `CSP_PING_RESULT` for that healthy success.

#### Scenario: Ping failure remains reviewable
- **WHEN** `CspBridge` completes a ping runtime call successfully but the ping
  result reports `success = false`
- **THEN** it SHALL still emit `CSP_PING_RESULT(targetNode, false, timeoutMs)`
- **AND** it SHALL preserve the existing failure/error signaling behavior.
