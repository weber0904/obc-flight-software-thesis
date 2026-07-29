## MODIFIED Requirements

### Requirement: Target/Lab UHF Uses Node 6 With Selectable Authority Roles
The target/lab physical-serial UHF path SHALL use `uhf_comm_csp_node` as CSP
node `6`, SHALL keep `uhf-backup` distinct from
`uhf-primary-after-failover`, and SHALL support UHF-primary behavior only after
explicit switch from the default S-band node-`5` path.

#### Scenario: UHF primary proof uses node 6 after explicit switch
- **WHEN** target launch, install, status, or probe surfaces use
  `TARGET_COMM_PROFILE=uhf-primary`
- **THEN** they SHALL configure `COMM_CSP_NODE=6`
- **AND** installed target OBC service bootstrap semantics SHALL keep
  `COMMAND_AUTHORITY_PROFILE=sband-primary`
- **AND** the bounded proof SHALL first use the default target node-`5` path to
  switch active COMM to `UHF primary`
- **AND** only after that switch SHALL node-`6` ingress exercise the
  `uhf-primary-after-failover` runtime role on ingress `1`

#### Scenario: UHF backup profile remains a separate bounded role
- **WHEN** target launch, install, status, or probe surfaces use
  `TARGET_COMM_PROFILE=uhf-backup`
- **THEN** they SHALL configure `COMM_CSP_NODE=6`
- **AND** installed target OBC service bootstrap semantics SHALL keep
  `COMMAND_AUTHORITY_PROFILE=sband-primary`
- **AND** node-`6` ingress SHALL expose `uhf-backup` authority semantics on
  ingress `1` without implying post-failover primary authority

#### Scenario: Target UHF peer stays on subsystem.local serial ingress
- **WHEN** the target/lab UHF profile is installed and started
- **THEN** `subsystem.local` SHALL run `uhf_comm_csp_node` as CSP node `6`
- **AND** it SHALL own the configured `/dev/serial0` lab ingress plus the
  shared SocketCAN COMM carrier
