## MODIFIED Requirements

### Requirement: Enveloped Commands Require Explicit Session Open

The command ingress authority gate SHALL require active secure authorization
state synthesized into internal opened-session truth before a secure command v2
packet may use sequence enforcement or reach `Svc::CmdDispatcher`, and current
runtime legacy command-envelope v1 lifecycle traffic SHALL fail closed before
dispatch, session mutation, or sequence mutation.

#### Scenario: Secure command v2 requires active auth state
- **WHEN** `CommandIngressAuthority` receives a valid secure command v2 packet
- **AND** there is no active secure authorization state for that
  `(ingressPort, serviceId)`
- **THEN** the gate SHALL reject the command before `Svc::CmdDispatcher`
- **AND** it SHALL NOT infer an active secure session from packet contents
  alone.

#### Scenario: Legacy v1 envelope traffic is unsupported on the current runtime
- **WHEN** `CommandIngressAuthority` receives a legacy command envelope v1
  packet on the current baseline
- **THEN** the gate SHALL reject that packet before `Svc::CmdDispatcher`
- **AND** it SHALL NOT create, replace, clear, or refresh session state
- **AND** it SHALL NOT consume or advance secure or legacy sequence state.

### Requirement: Session Open Is The V1 Lifecycle And Recovery Surface

Current runtime session establishment SHALL be driven by secure-auth grant
events rather than by public `SESSION_OPEN`, and any retained legacy
`SESSION_OPEN` semantics SHALL remain historical compatibility evidence only
instead of an accepted current lifecycle or recovery surface.

#### Scenario: Auth success synthesizes secure runtime open
- **WHEN** `SecureLinkAuthorizer` reports `authGranted(ingressPort, serviceId,
  sKey)`
- **THEN** `CommandIngressAuthority` SHALL establish an active secure session
  for that `(ingressPort, serviceId)`
- **AND** it SHALL establish secure sequence baseline `0`
- **AND** it SHALL emit the same runtime opened-session side effects needed by
  the current observer path without requiring a wire-level `SESSION_OPEN`.

#### Scenario: Public legacy SESSION_OPEN is unsupported
- **WHEN** an operator, helper, or probe attempts to use public
  `SESSION_OPEN` as a current runtime lifecycle surface
- **THEN** the current baseline SHALL reject that path
- **AND** it SHALL NOT establish session, persistence, or reopen-floor state
  from that request.

### Requirement: Session Lifecycle Remains Distinct From Legacy Commands

Current runtime session lifecycle SHALL remain an internal secure-session
contract driven by secure-auth grant and revoke, and legacy command-path
traffic SHALL remain outside that lifecycle state.

#### Scenario: Auth revoke clears the active secure session
- **WHEN** `SecureLinkAuthorizer` reports secure-session revocation or timeout
- **THEN** `CommandIngressAuthority` SHALL clear the active secure session for
  the affected `(ingressPort, serviceId)`
- **AND** it SHALL preserve the current runtime revoke side effects and
  telemetry or event evidence.

#### Scenario: Legacy commands remain outside current session lifecycle
- **WHEN** `CommandIngressAuthority` receives legacy lifecycle or non-lifecycle
  command traffic on the current baseline
- **THEN** it SHALL reject that traffic before dispatch
- **AND** it SHALL NOT create, replace, clear, or consume current session
  lifecycle state.

### Requirement: Session Lifecycle Evidence Is Dedicated And Bounded

Session lifecycle SHALL expose dedicated secure-session open, revoke, and
reject evidence for the current secure baseline without presenting legacy
persistent freshness or reopen-floor behavior as active runtime truth.

#### Scenario: Secure-session lifecycle evidence stays reviewable
- **WHEN** the current secure baseline opens, revokes, or rejects secure
  command activity
- **THEN** it SHALL emit dedicated reviewable event and telemetry evidence
  including opened-session, revoked-session, accepted-sequence, and reject
  counters
- **AND** it SHALL keep that evidence distinct from `BootManager` boot metadata
  truth.

#### Scenario: Historical legacy lifecycle claims stay archived only
- **WHEN** historical compatibility evidence cites legacy `SESSION_OPEN` or
  reopen-floor persistence
- **THEN** that citation SHALL remain reviewable as archived evidence only
- **AND** it SHALL NOT be treated as current runtime replay or lifecycle truth.

### Requirement: Reboot Requires Fresh Session Open

Runtime restart SHALL clear active secure session state and require a fresh
secure-auth cycle before later secure commands may dispatch, and the current
baseline SHALL NOT depend on persisted legacy reopen-floor semantics after
restart.

#### Scenario: Restart clears active secure session state
- **WHEN** the hosted runtime or equivalent topology process restarts
- **THEN** all active in-memory secure session state and secure sequence state
  SHALL be cleared
- **AND** later secure command traffic SHALL fail closed until a fresh secure
  auth grant re-establishes the session.

#### Scenario: Legacy reopen-floor persistence is not part of current baseline
- **WHEN** reviewers inspect current restart behavior
- **THEN** they SHALL find current runtime restart closure described in terms
  of fresh secure-auth re-bootstrap
- **AND** they SHALL NOT find persisted legacy `SESSION_OPEN` reopen-floor
  semantics described as current maintained runtime contract.

### Requirement: Command Envelope V1 Supports Authenticated Source Binding

The repository SHALL keep `source_id`, `key_slot`, and `session_id` as
historical legacy v1 envelope vocabulary only, while the current secure command
v2 property SHALL bind trust to the active secure authorization state keyed by
ingress and service.

#### Scenario: Secure command v2 omits source and key-slot fields
- **WHEN** `CommandIngressAuthority` receives a secure command v2 candidate
- **THEN** the secure command v2 contract SHALL include only secure sequence
  number, inner command payload, and MAC material
- **AND** it SHALL bind trust to the previously granted active secure auth
  state for that `(ingressPort, serviceId)`
- **AND** it SHALL NOT require wire-level `source_id`, `key_slot`, or
  `session_id` fields.

#### Scenario: Legacy tuple fields remain historical only
- **WHEN** current runtime docs, configs, or proofs describe command-ingress
  trust anchors
- **THEN** they SHALL use the secure-auth state keyed by ingress and service
- **AND** any legacy `source_id`, `key_slot`, or `session_id` discussion SHALL
  be explicitly historical or compatibility-only.
