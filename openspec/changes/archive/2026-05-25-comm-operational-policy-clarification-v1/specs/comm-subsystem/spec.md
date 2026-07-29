## ADDED Requirements

### Requirement: Current COMM Policy Keeps Transport Facts, Runtime Policy, And Future Reliable Transfer Separate

The comm subsystem SHALL describe the current baseline using three explicit
layers: raw observation or transport fact, session or link-role runtime policy,
and future reliable-transfer behavior.

#### Scenario: Current COMM wording does not collapse adjacent layers
- **WHEN** reviewers inspect current COMM architecture, runbooks, or interface
  wording
- **THEN** raw `GroundLinkDriver`, `UartDriver`, and `RadioController`
  observations SHALL remain transport or observation facts
- **AND** `CommandIngressAuthority` and `CommController` decisions SHALL remain
  runtime policy rather than raw transport truth
- **AND** reliable transfer, ARQ, NACK, packet retry, or CFDP behavior SHALL
  remain future scope unless separate governed evidence proves it

### Requirement: Accepted SESSION_OPEN(seq0) Is The Current UHF Operational Session Boundary

The current comm-managed UHF command-session boundary SHALL begin only after an
authority-allowed valid `SESSION_OPEN` command envelope is accepted with
`sequence_number = 0`.

#### Scenario: Link acquisition alone is not a command-session boundary
- **WHEN** the UHF link is electrically present, bytes are exchanged, or a
  first non-lifecycle command succeeds on an adjacent path
- **THEN** the repository SHALL NOT describe that fact alone as the current UHF
  operational command-session boundary
- **AND** the current policy-entry boundary SHALL remain accepted
  `SESSION_OPEN(seq0)` on the relevant comm-managed ingress

### Requirement: Beacon Suppress And Resume Policy Belongs To CommController

The current COMM policy SHALL assign beacon suppress and resume ownership to
`CommController`, SHALL treat accepted `SESSION_OPEN(seq0)` as the suppress
start boundary for UHF command-session policy, and SHALL treat bounded
inactivity timeout as the intended resume boundary.

#### Scenario: Beacon publisher does not own command-session arbitration
- **WHEN** reviewers inspect beacon/session policy wording
- **THEN** `BeaconPublisher` SHALL remain the bounded cadence and encode owner
- **AND** it SHALL NOT be described as the authority that decides when command
  session activity suppresses or resumes UHF beaconing

#### Scenario: Beacon suppress runtime remains a current non-claim
- **WHEN** current code or evidence is cited
- **THEN** the repository SHALL describe beacon suppress and resume as a
  governing policy rule for future runtime alignment
- **AND** it SHALL NOT claim that session-aware beacon suppression is already
  implemented or formally proven on the current baseline

### Requirement: Current COMM Relay Policy Is Single-Path With Explicit Switch

The current comm runtime policy SHALL treat S-band node `5` as the default
active path, SHALL treat UHF node `6` as either bounded `uhf-backup` ingress or
explicit-switch `uhf-primary-after-failover`, and SHALL NOT claim simultaneous
routed S-band and UHF relay ownership on one active baseline process.

#### Scenario: Dual-link behavior stays outside current runtime claim
- **WHEN** the repository describes current hosted or target/lab COMM runtime
- **THEN** it SHALL describe the active policy as one active relay path plus
  explicit switch or failover boundaries
- **AND** it SHALL keep simultaneous dual-link arbitration, concurrent
  multi-owner relay, and always-on split-link runtime guarantees as future work

### Requirement: Current Retry Responsibility Stays Ground-Side And Whole-Command Only

The current baseline SHALL keep retry responsibility limited to bounded
ground-side or probe-side whole-command retries and SHALL NOT describe current
COMM runtime policy as packet retry, file-packet retry, or reliable-transfer
ownership.

#### Scenario: Current retry wording avoids transfer-scope inflation
- **WHEN** a current COMM path or operator flow retries a command or file
  command
- **THEN** that retry SHALL be described as a whole-command resend helper on
  the ground or probe side
- **AND** the wording SHALL NOT imply current packet retransmission,
  link-runtime ARQ, or reliable-transfer closure

## MODIFIED Requirements

### Requirement: Service-Managed Target/Lab UHF Path Exists

The comm subsystem SHALL support a service-managed target/lab UHF path where
`subsystem.local` hosts `uhf_comm_csp_node` as COMM node `6` on the physical
serial ingress while preserving the existing COMM service contract, and current
operator profile naming SHALL remain distinct from the post-switch runtime role
name.

#### Scenario: Target UHF service owns node 6
- **WHEN** the target/lab UHF COMM profile is installed and enabled
- **THEN** `subsystem.local` SHALL run `uhf_comm_csp_node` as COMM node `6`
- **AND** it SHALL use the configured lab serial ingress device and baudrate
  plus the existing COMM services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and
  `32 LINK_STATUS`

#### Scenario: Target UHF profiles select authority role without changing node identity
- **WHEN** target/lab UHF runs as operator profile `uhf-primary` or
  `uhf-backup`
- **THEN** both profiles SHALL keep `COMM_CSP_NODE=6`
- **AND** installed target OBC bootstrap semantics SHALL keep
  `COMMAND_AUTHORITY_PROFILE=sband-primary`
- **AND** operator profile `uhf-primary` SHALL exercise node-`6` ingress only
  after explicit switch from the default node-`5` path
- **AND** that switched role SHALL be the current
  `uhf-primary-after-failover` runtime policy role
- **AND** `uhf-backup` SHALL exercise node-`6` ingress as bounded backup
  authority on ingress `1`
