## ADDED Requirements

### Requirement: COMM Policy Clarifications Cite Governing Evidence Or Stay Non-Claims

The verification-path registry SHALL require current COMM policy wording to
cite the governing archived evidence for any claimed path behavior and SHALL
keep unproven adjacent policy points explicit as non-claims, bounded
assumptions, or future follow-up.

#### Scenario: Clarified COMM policy stays tied to proven paths
- **WHEN** the repository cites current behavior for `SESSION_OPEN(seq0)`,
  `uhf-backup`, `uhf-primary-after-failover`, hosted beacon side-channel
  capture, gateway-backed S-band/UHF paths, or target node-`5`/node-`6`
  operation
- **THEN** the wording SHALL point to the governing registry entries or
  archived evidence records for those exact paths
- **AND** it SHALL keep simultaneous dual-link runtime, session-aware beacon
  suppress runtime, and reliable-transfer behavior outside the proven-path
  claim unless separate evidence proves them

## MODIFIED Requirements

### Requirement: Target Node-6 Quiet UHF Paths Are Registered Separately
The verification-path registry SHALL register target node-`6`
`uhf-primary-after-failover` and `uhf-backup` command/readback proofs as
bounded quiet-mode paths distinct from both the default target node-`5` path
and hosted UHF evidence.

#### Scenario: Registry names the quiet node-6 boundary
- **WHEN** target node-`6` quiet-mode evidence passes
- **THEN** the registry SHALL identify the path as
  `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> physical serial -> subsystem.local uhf_comm_csp_node(node 6) -> shared SocketCAN -> obc.local OBC`
- **AND** it SHALL record whether the bounded proof used
  `uhf-primary-after-failover` or `uhf-backup`
- **AND** it SHALL record that operator profile `uhf-primary` proves the
  `uhf-primary-after-failover` runtime role only after explicit switch from the
  default target node-`5` path
