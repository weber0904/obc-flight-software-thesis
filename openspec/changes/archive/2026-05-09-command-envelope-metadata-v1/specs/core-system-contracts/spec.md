## ADDED Requirements

### Requirement: Command Envelope Metadata Is Carried In The Routed Command Path
The command ingress authority gate SHALL support a project-owned command envelope v1 carried as an outer F Prime command packet before `Svc::CommandDispatcher`.

#### Scenario: Envelope uses a project pseudo-opcode
- **WHEN** a routed `Fw.Com` command packet reaches `CommandIngressAuthority`
- **AND** the outer command opcode is `0x0BC10001`
- **THEN** the packet SHALL be treated as a mission command envelope v1 candidate
- **AND** it SHALL NOT be forwarded to `Svc::CommandDispatcher` as that pseudo-opcode.

#### Scenario: Envelope header is parsed before authority evaluation
- **WHEN** an envelope candidate contains magic `0x0BC0DE01`, version `1`, flags `0`, header length `20`, and reserved field `0`
- **THEN** the gate SHALL parse `sessionId`, `sequenceNumber`, `innerLength`, and the complete inner serialized `Fw.CmdPacket`
- **AND** authority evaluation SHALL use the inner command opcode.

#### Scenario: Legacy commands remain supported
- **WHEN** a routed command packet does not use the envelope pseudo-opcode
- **THEN** `CommandIngressAuthority` SHALL process it through the existing legacy authority path
- **AND** the existing `fprime-cli -> GDS -> CCSDS -> OBC` command path SHALL remain supported.

### Requirement: Envelope Metadata Is Observable But Not Enforcing
The command envelope v1 SHALL expose mission metadata for later session/sequence enforcement without changing runtime command acceptance based on sequence values.

#### Scenario: Valid envelope metadata is observed
- **WHEN** a valid envelope is accepted for authority evaluation
- **THEN** the gate SHALL emit `COMMAND_ENVELOPE_OBSERVED` with ingress port, configured link identity, configured link role, session ID, sequence number, and inner opcode
- **AND** it SHALL update bounded envelope telemetry with the latest session ID, sequence number, and inner opcode.

#### Scenario: Sequence values do not enforce in v1
- **WHEN** two envelopes for the same configured ingress source carry duplicate, lower, or wraparound sequence numbers
- **THEN** v1 SHALL NOT reject either envelope because of sequence ordering alone
- **AND** it SHALL leave active sequence enforcement to a later governed change.

#### Scenario: Context remains status correlation only
- **WHEN** an envelope command is forwarded or rejected
- **THEN** `Fw.Com.context` SHALL be preserved unchanged for command-status correlation
- **AND** it SHALL NOT be treated as session identity, sequence number, source identity, or authority evidence.

### Requirement: Malformed Envelopes Fail Closed
Recognized command envelope candidates that cannot be parsed as valid v1 envelopes SHALL fail closed before `Svc::CommandDispatcher`.

#### Scenario: Malformed envelope is rejected before dispatch
- **WHEN** an envelope candidate has bad magic, unsupported version, nonzero flags, wrong header length, nonzero reserved field, truncated header, truncated inner command, or oversized inner length
- **THEN** the gate SHALL NOT forward the candidate to `Svc::CommandDispatcher`
- **AND** it SHALL return exactly one synthetic `Fw.CmdResponse::FORMAT_ERROR` through the same port-indexed status path
- **AND** it SHALL emit `COMMAND_ENVELOPE_REJECTED`.

#### Scenario: Malformed envelope response opcode is deterministic
- **WHEN** a malformed envelope contains enough bytes to safely decode the inner command opcode
- **THEN** the synthetic status SHALL use the inner opcode
- **OTHERWISE** the synthetic status SHALL use `0xFFFFFFFF`.

### Requirement: Envelope Authority Uses Existing Ingress Policy
The command envelope v1 SHALL not introduce a new authority source or bypass existing command ingress authority policy.

#### Scenario: Envelope source is not packet metadata
- **WHEN** an envelope is evaluated
- **THEN** the configured authority source SHALL be selected only from the `CommandIngressAuthority` input port index
- **AND** the envelope SHALL NOT carry or override source identity.

#### Scenario: Restricted envelope command is denied by existing policy
- **WHEN** `uhf-backup` ingress receives an envelope whose inner command is outside the v1 allowlist
- **THEN** the gate SHALL deny the command before `Svc::CommandDispatcher`
- **AND** it SHALL produce the same authority rejection semantics as a legacy command with that inner opcode.
