## ADDED Requirements

### Requirement: Enveloped Commands Enforce Session Sequence Before Dispatch

The command ingress authority gate SHALL enforce strict-monotonic sequence state for valid command envelope v1 packets before forwarding an authority-allowed inner command to `Svc::CmdDispatcher`.

#### Scenario: Valid envelope uses authority-then-sequence ordering
- **WHEN** `CommandIngressAuthority` receives a valid command envelope v1 packet
- **THEN** it SHALL parse and observe the envelope metadata
- **AND** it SHALL evaluate existing command authority policy against the inner command opcode before evaluating sequence state
- **AND** it SHALL evaluate sequence state only when the inner command is allowed by authority policy.

#### Scenario: Accepted sequence forwards inner command
- **WHEN** an authority-allowed valid envelope carries the first or an increasing sequence number for its session key
- **THEN** `CommandIngressAuthority` SHALL accept the sequence
- **AND** it SHALL forward exactly one inner command buffer to `Svc::CmdDispatcher`
- **AND** it SHALL preserve the original `Fw.Com.context` value for command status correlation.

#### Scenario: Rejected sequence does not dispatch
- **WHEN** an authority-allowed valid envelope carries a duplicate, lower, wraparound, or table-full sequence result
- **THEN** `CommandIngressAuthority` SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL forward zero inner command buffers
- **AND** it SHALL emit exactly one synthetic command response.

### Requirement: Command Sequence Enforcement Uses Configured Source Key

Active command sequence enforcement SHALL key runtime sequence state by configured ingress source and envelope session ID.

#### Scenario: Session key is derived from configured ingress source
- **WHEN** `CommandIngressAuthority` evaluates sequence state for a valid envelope
- **THEN** the session key SHALL include ingress port, configured link identity, configured link role, and envelope session ID
- **AND** it SHALL NOT use `Fw.Com.context`, gateway metadata, packet body source claims, or inner command arguments as source identity.

#### Scenario: Source/session/role keys are independent
- **WHEN** the same sequence number appears under a different ingress port, configured link identity, configured link role, or session ID
- **THEN** sequence enforcement SHALL treat it as a separate key.

### Requirement: Command Sequence Rejection Has Dedicated Evidence

Active command sequence rejection SHALL expose bounded event, telemetry, and command-response evidence without reusing command authority rejection reasons.

#### Scenario: Duplicate or lower sequence rejection is validation error
- **WHEN** a valid envelope is rejected because its sequence number is not greater than the last accepted sequence for the same key
- **THEN** the synthetic command response SHALL be `Fw::CmdResponse::VALIDATION_ERROR`
- **AND** the sequence rejection reason SHALL be `NOT_INCREASING`.

#### Scenario: Window full rejection is execution error
- **WHEN** a valid envelope is rejected because the sequence window cannot allocate a new session entry
- **THEN** the synthetic command response SHALL be `Fw::CmdResponse::EXECUTION_ERROR`
- **AND** the sequence rejection reason SHALL be `WINDOW_FULL`.

#### Scenario: Sequence rejection evidence is bounded
- **WHEN** sequence rejection occurs
- **THEN** `CommandIngressAuthority` SHALL emit `COMMAND_SEQUENCE_REJECTED` with ingress port, link identity, link role, session ID, sequence number, inner opcode, reason, and response
- **AND** it SHALL update bounded sequence rejection telemetry including total count, per-reason counts, and last rejected source/session/sequence/opcode/reason.

### Requirement: Sequence Enforcement Does Not Add Session Lifecycle Or Replay Protection Claims

`command-session-sequence-v1` SHALL remain a runtime duplicate/lower sequence rejection primitive and SHALL NOT claim full replay protection or session lifecycle management.

#### Scenario: Legacy and denied commands do not update sequence state
- **WHEN** a command is a legacy non-envelope command, a malformed envelope, an invalid-config command, an unknown/restricted command, or an authority-denied envelope
- **THEN** the command SHALL NOT update active sequence state.

#### Scenario: Runtime reset remains deferred
- **WHEN** `command-session-sequence-v1` is cited
- **THEN** it SHALL state that no production runtime session reset, session-open, resync, persistent session state, authentication, crypto, nonce, MAC, reliable transfer, file authority, unknown packet authority, trusted source, physical UHF provenance, or dual-link simultaneous proof is provided.
