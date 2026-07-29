## ADDED Requirements

### Requirement: Command Envelope V1 Supports Authenticated Source Binding

The command ingress authority gate SHALL support authenticated command envelope v1 traffic that binds configured source identity, session state, sequence state, and inner command integrity before policy or dispatch.

#### Scenario: Authenticated envelope carries source and key binding
- **WHEN** `CommandIngressAuthority` receives a command envelope v1 candidate
- **THEN** the envelope contract SHALL include source identity, key-slot selection, session ID, sequence number, inner command payload, and MAC material sufficient for authenticated verification
- **AND** the MAC input SHALL cover protocol/version fields plus those authenticated fields.

#### Scenario: Configured ingress source remains trust anchor
- **WHEN** `CommandIngressAuthority` evaluates authenticated envelope source identity
- **THEN** it SHALL cross-check envelope source identity and key-slot fields against the configured ingress source for that input port
- **AND** it SHALL NOT infer trust from `Fw.Com.context`, gateway metadata, or packet-body source claims alone.

### Requirement: Auth Verification Precedes Policy And State Mutation

Authenticated envelope verification SHALL occur before authority, lifecycle, sequence, or dispatch decisions for enveloped traffic.

#### Scenario: Valid enveloped traffic uses parse-auth-policy ordering
- **WHEN** `CommandIngressAuthority` receives an enveloped command candidate
- **THEN** it SHALL process that traffic in this order:
  1. parse envelope
  2. verify auth
  3. evaluate authority
  4. evaluate lifecycle
  5. evaluate sequence
  6. dispatch

#### Scenario: Auth failure rejects before policy or session logic
- **WHEN** envelope parse or auth verification fails
- **THEN** `CommandIngressAuthority` SHALL reject the command before authority, lifecycle, sequence, or dispatch
- **AND** it SHALL forward zero inner command buffers.

### Requirement: Auth, Lifecycle, And Sequence Mutations Stay Separate

Authenticated envelope rejection paths SHALL keep auth, lifecycle, and sequence state boundaries explicit.

#### Scenario: Auth failure does not mutate runtime session state
- **WHEN** envelope parse or auth verification fails
- **THEN** the gate SHALL NOT open a session
- **AND** it SHALL NOT mutate active session metadata
- **AND** it SHALL NOT consume sequence state.

#### Scenario: Authority denial after auth does not mutate lifecycle or sequence state
- **WHEN** auth succeeds but authority denies the inner command
- **THEN** the gate SHALL NOT open a session implicitly
- **AND** it SHALL NOT mutate active session metadata
- **AND** it SHALL NOT consume sequence state.

#### Scenario: Lifecycle denial does not consume sequence state
- **WHEN** auth succeeds, authority allows, and lifecycle validation rejects the envelope
- **THEN** the gate SHALL reject before dispatch
- **AND** it SHALL NOT consume sequence state.

### Requirement: Authenticated Envelope Evidence Is Distinct From Raw Observation

Authenticated command acceptance SHALL be distinguishable from low-level envelope detection or parse-only observation.

#### Scenario: Auth-failed traffic is not presented as authenticated acceptance
- **WHEN** auth verification fails
- **THEN** runtime evidence SHALL NOT report that command as authority-accepted, lifecycle-valid, or session-observed in a way that can be confused with authenticated acceptance.

### Requirement: Legacy Routed Commands Remain Explicit Compatibility Path

Legacy non-envelope routed commands MAY remain supported during authenticated envelope adoption, but they SHALL remain outside the authenticated command-security claim.

#### Scenario: Legacy compatibility does not inherit authenticated privileges
- **WHEN** `CommandIngressAuthority` receives a legacy non-envelope routed command
- **THEN** it SHALL preserve the existing legacy authority behavior
- **AND** it SHALL NOT treat that traffic as authenticated
- **AND** it SHALL NOT silently inherit authenticated session semantics or authenticated-evidence meaning.

### Requirement: Authenticated Envelope Claims Stay Bounded

`command-auth-envelope-v1` SHALL provide authenticated ingress foundation only and SHALL NOT over-claim full replay or secure-persistence behavior.

#### Scenario: Authenticated ingress does not imply full replay protection
- **WHEN** `command-auth-envelope-v1` is cited
- **THEN** it SHALL be valid to claim authenticated command ingress foundation with source-bound MAC verification and fail-closed auth rejection
- **AND** it SHALL NOT be cited as full replay protection, nonce-window enforcement, persistent anti-replay state, persistent secure key storage, file authority, unknown packet authority, trusted boot chain, or simultaneous dual-link proof.
