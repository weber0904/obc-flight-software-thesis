## ADDED Requirements

### Requirement: Command Session Sequence Foundation Is Defined
The core system contract SHALL define an inactive command session/sequence helper foundation for future command envelope and authentication work.

#### Scenario: Session key separates configured source and authority epoch
- **WHEN** the session sequence helper is used by tests or future callers
- **THEN** its key SHALL include ingress port, link identity, link role, and caller-provided session ID
- **AND** link role changes SHALL be treated as a separate authority epoch.

#### Scenario: Session ID is caller provided in v1
- **WHEN** `command-session-sequence-foundation-v1` is cited
- **THEN** it SHALL state that v1 does not generate, parse, transmit, persist, or authenticate session IDs
- **AND** it SHALL NOT claim an active command wire format or session-open protocol.

### Requirement: Strict Monotonic Sequence Helper Is Provided
The command session sequence foundation SHALL provide a strict monotonic sequence helper that can later be called by a command envelope/authentication layer.

#### Scenario: Increasing sequence is accepted
- **WHEN** a session key has no accepted sequence
- **THEN** the first sequence number SHALL be accepted
- **WHEN** a later sequence number is greater than the last accepted sequence for that key
- **THEN** it SHALL be accepted and become the new last accepted sequence.

#### Scenario: Duplicate and lower sequence are rejected
- **WHEN** a sequence number is equal to or lower than the last accepted sequence for the same key
- **THEN** the helper SHALL reject it
- **AND** it SHALL NOT update the last accepted sequence.

#### Scenario: Reset is explicit
- **WHEN** a caller resets a session key
- **THEN** the next sequence number for that key SHALL establish a new baseline
- **AND** sequence wraparound SHALL NOT be automatically accepted without such reset.

#### Scenario: Foundation is not replay protection
- **WHEN** this helper is documented or cited
- **THEN** it SHALL be described only as a future replay/duplicate rejection primitive
- **AND** it SHALL NOT be described as active runtime enforcement, replay protection, authentication, nonce validation, or persistent anti-replay state.
