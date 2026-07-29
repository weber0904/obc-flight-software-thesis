## MODIFIED Requirements

### Requirement: Enveloped Commands Require Explicit Session Open

The command ingress authority gate SHALL require explicit lifecycle open only
for legacy command envelope v1 traffic; the new secure command v2 path SHALL
instead require an active secure authorization state synthesized into internal
session-open truth before a non-lifecycle command may use sequence enforcement
or reach `Svc::CmdDispatcher`.

#### Scenario: Legacy v1 keeps explicit lifecycle open
- **WHEN** `CommandIngressAuthority` receives a valid authority-allowed
  command envelope v1 whose inner opcode is not `SESSION_OPEN`
- **AND** the v1 source epoch has no active lifecycle session
- **THEN** the gate SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL NOT create legacy session or sequence state implicitly from
  that packet.

#### Scenario: Secure command v2 requires active auth state
- **WHEN** `CommandIngressAuthority` receives a valid secure command v2 packet
- **AND** there is no active secure authorization state for that
  `(ingressPort, serviceId)`
- **THEN** the gate SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL NOT infer an active secure session from packet contents
  alone.

### Requirement: Session Open Is The V1 Lifecycle And Recovery Surface

`SESSION_OPEN` SHALL remain the only command-session open, replace, and resync
surface for legacy command envelope v1 traffic and SHALL NOT be required on the
new challenge-authenticated secure command v2 path.

#### Scenario: Legacy fresh open remains v1 lifecycle
- **WHEN** a legacy v1 source epoch receives an authority-allowed valid
  `SESSION_OPEN` envelope with `sequence_number = 0`
- **THEN** the gate SHALL preserve the existing v1 reopen-floor and active
  lifecycle behavior for that path.

#### Scenario: Auth success synthesizes secure runtime open
- **WHEN** `SecureLinkAuthorizer` reports `authGranted(ingressPort, serviceId,
  sKey)`
- **THEN** `CommandIngressAuthority` SHALL establish an active secure session
  for that `(ingressPort, serviceId)`
- **AND** it SHALL establish secure sequence baseline `0`
- **AND** it SHALL emit the same runtime opened-session side effects needed by
  the current observer path without requiring a wire-level `SESSION_OPEN`.

### Requirement: Command Envelope V1 Supports Authenticated Source Binding

The repository SHALL keep authenticated source binding as a legacy v1 property
and SHALL add a distinct secure command v2 property whose trust anchor is the
active secure authorization state keyed by ingress and service rather than by
wire-level source or key-slot fields.

#### Scenario: Legacy v1 keeps source and key binding
- **WHEN** `CommandIngressAuthority` receives a command envelope v1 candidate
- **THEN** the envelope contract SHALL continue to include source identity,
  key-slot selection, session ID, sequence number, inner command payload, and
  MAC material sufficient for legacy authenticated verification.

#### Scenario: Secure command v2 omits source and key-slot fields
- **WHEN** `CommandIngressAuthority` receives a secure command v2 candidate
- **THEN** the secure command v2 contract SHALL include only secure sequence
  number, inner command payload, and MAC material
- **AND** it SHALL bind trust to the previously granted active secure auth
  state for that `(ingressPort, serviceId)`
- **AND** it SHALL NOT require wire-level `source_id`, `key_slot`, or
  `session_id` fields.

### Requirement: Auth Verification Precedes Policy And State Mutation

Authenticated command processing SHALL remain parse-auth-policy ordered on both
legacy v1 and secure command v2 paths, with secure command v2 additionally
requiring a previously granted secure authorization state before policy or
dispatch may proceed.

#### Scenario: Secure command v2 uses parse-auth-policy ordering
- **WHEN** `CommandIngressAuthority` receives a secure command v2 candidate
- **THEN** it SHALL process that traffic in this order:
  1. parse secure command v2
  2. verify an active secure authorization state exists
  3. verify secure command MAC
  4. evaluate authority
  5. evaluate sequence
  6. dispatch

#### Scenario: Secure MAC failure rejects before policy or sequence mutation
- **WHEN** secure command v2 parse fails, active auth state is absent, or MAC
  verification fails
- **THEN** `CommandIngressAuthority` SHALL reject the command before
  authority, sequence, or dispatch
- **AND** it SHALL forward zero inner command buffers.

### Requirement: Auth, Lifecycle, And Sequence Mutations Stay Separate

Authenticated rejection paths SHALL keep secure auth state, secure sequence
state, and legacy lifecycle state boundaries explicit.

#### Scenario: Secure auth revoke clears secure state without mutating legacy v1
- **WHEN** `CommandIngressAuthority` receives `authRevoked(ingressPort,
  serviceId, reason)`
- **THEN** it SHALL clear the active secure session for that
  `(ingressPort, serviceId)`
- **AND** it SHALL leave any legacy v1 lifecycle state on adjacent ingress
  paths unchanged.

#### Scenario: Secure authority denial does not consume secure sequence
- **WHEN** secure command v2 MAC verification succeeds but authority denies the
  inner command
- **THEN** the gate SHALL reject before dispatch
- **AND** it SHALL NOT consume secure sequence state.

### Requirement: Authenticated Envelope Claims Stay Bounded

The new challenge-authenticated secure command path SHALL provide authenticated
secure session bootstrap plus per-session command integrity and SHALL NOT
over-claim encryption, hardware-backed storage, or full replay protection.

#### Scenario: Secure challenge auth stays bounded
- **WHEN** the secure challenge-authenticated command path is cited
- **THEN** it SHALL be valid to claim service-bound challenge/response session
  bootstrap, active-session HMAC command verification, and strict-monotonic
  per-session sequence rejection
- **AND** it SHALL NOT be cited as encryption, persistent secure key storage,
  hardware-backed security, file authority, unknown packet authority, or full
  replay protection outside the active secure session.
