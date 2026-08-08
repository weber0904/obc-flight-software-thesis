## ADDED Requirements

### Requirement: Enveloped Commands Require Explicit Session Open

The command ingress authority gate SHALL require an explicit session-open transition before a non-lifecycle command envelope may use active sequence enforcement or reach `Svc::CmdDispatcher`.

#### Scenario: First non-lifecycle packet is not an implicit open
- **WHEN** `CommandIngressAuthority` receives a valid authority-allowed command envelope whose inner opcode is not `SESSION_OPEN`
- **AND** the source epoch has no active session
- **THEN** the gate SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL NOT create session or sequence state implicitly from that packet.

#### Scenario: Active session must match command envelope
- **WHEN** `CommandIngressAuthority` receives a valid authority-allowed command envelope whose inner opcode is not `SESSION_OPEN`
- **AND** the source epoch has an active session with a different `session_id`
- **THEN** the gate SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL leave the active session and sequence state unchanged.

### Requirement: Session Open Is The V1 Lifecycle And Recovery Surface

`SESSION_OPEN` SHALL be the only v1 command-session open, replace, and resync surface.

#### Scenario: Open requires valid envelope and zero sequence
- **WHEN** `CommandIngressAuthority` receives an envelope whose inner opcode is `SESSION_OPEN`
- **THEN** the envelope SHALL be valid command envelope v1
- **AND** the envelope sequence number SHALL be `0`
- **AND** the gate SHALL reject nonzero open sequence numbers before `Svc::CmdDispatcher`.

#### Scenario: Fresh open establishes active session
- **WHEN** a source epoch has no active session
- **AND** it receives an authority-allowed valid `SESSION_OPEN` envelope with `sequence_number = 0`
- **THEN** the gate SHALL establish that envelope `session_id` as the active session for the source epoch
- **AND** it SHALL establish sequence baseline `0`
- **AND** it SHALL synthesize exactly one `Fw::CmdResponse::OK` without forwarding the inner command to `Svc::CmdDispatcher`.

#### Scenario: Fresh open replaces a prior session on the same source epoch
- **WHEN** a source epoch already has an active session
- **AND** it receives an authority-allowed valid `SESSION_OPEN` envelope with a different `session_id`
- **THEN** the gate SHALL replace the prior active session atomically
- **AND** it SHALL discard the prior session's sequence state
- **AND** it SHALL establish the new session baseline at sequence `0`.

#### Scenario: Same-session reopen fails closed
- **WHEN** a source epoch already has an active session
- **AND** it receives an authority-allowed valid `SESSION_OPEN` envelope with the same `session_id`
- **THEN** the gate SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL leave lifecycle and sequence state unchanged.

### Requirement: Session Lifecycle Remains Distinct From Legacy Commands

The explicit session lifecycle SHALL remain envelope-only in v1.

#### Scenario: Legacy lifecycle command is rejected before dispatch
- **WHEN** `CommandIngressAuthority` receives a legacy non-envelope command whose opcode is `SESSION_OPEN`
- **THEN** the gate SHALL reject that command before `Svc::CmdDispatcher`
- **AND** it SHALL NOT treat the legacy command path as a valid lifecycle surface.

#### Scenario: Legacy non-lifecycle commands remain outside session state
- **WHEN** `CommandIngressAuthority` receives a legacy non-envelope command whose opcode is not `SESSION_OPEN`
- **THEN** it SHALL preserve the existing legacy authority behavior
- **AND** it SHALL NOT create, replace, clear, or consume session lifecycle state.

### Requirement: Session Lifecycle Evidence Is Dedicated And Bounded

Session lifecycle SHALL expose dedicated rejection and open evidence without expanding broader security claims.

#### Scenario: Session lifecycle emits dedicated events and telemetry
- **WHEN** a session open or lifecycle rejection occurs
- **THEN** `CommandIngressAuthority` SHALL emit dedicated session lifecycle event evidence
- **AND** it SHALL update bounded telemetry for active session state, last accepted sequence, open counts, and lifecycle rejection evidence.

#### Scenario: Lifecycle state remains runtime-memory only
- **WHEN** `command-session-lifecycle-v1` is cited
- **THEN** it SHALL state that session lifecycle state is runtime memory only
- **AND** it SHALL NOT claim persistent secure storage, authenticated replay protection, crypto authentication, trusted source identity, file authority, unknown packet authority, or dual-link simultaneous proof.

### Requirement: Reboot Requires Fresh Session Open

Session lifecycle state SHALL be cleared by runtime restart unless a later governed change adds persistence.

#### Scenario: Runtime restart clears active session state
- **WHEN** the hosted runtime or equivalent topology process restarts
- **THEN** all active session lifecycle state and in-memory sequence state SHALL be cleared
- **AND** non-lifecycle enveloped traffic SHALL fail closed until a fresh `SESSION_OPEN` is accepted.
