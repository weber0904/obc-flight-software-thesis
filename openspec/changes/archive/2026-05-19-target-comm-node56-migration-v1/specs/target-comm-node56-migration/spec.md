## ADDED Requirements

### Requirement: Target/Lab COMM Uses Node 5 As The Default Active Path
The target/lab COMM baseline SHALL retire active operator use of generic COMM node `4` and SHALL treat S-band node `5` as the default target path.

#### Scenario: Default target profile selects node 5
- **WHEN** target OBC launch, install, status, or probe surfaces use the default target COMM profile
- **THEN** they SHALL configure `COMM_CSP_NODE=5`
- **AND** they SHALL configure `COMMAND_AUTHORITY_PROFILE=sband-primary`

#### Scenario: Target S-band peer runs on subsystem.local
- **WHEN** the target/lab S-band profile is installed and started
- **THEN** `subsystem.local` SHALL run `sband_comm_csp_node` as CSP node `5`
- **AND** that service SHALL expose a TCP listener while keeping internal CSP traffic on the shared SocketCAN carrier

### Requirement: Target/Lab UHF Uses Node 6 With Selectable Authority Roles
The target/lab physical-serial UHF path SHALL use `uhf_comm_csp_node` as CSP node `6` and SHALL support both `uhf-primary` and `uhf-backup` authority-role profiles.

#### Scenario: UHF primary proof uses node 6 after explicit switch
- **WHEN** target launch, install, status, or probe surfaces use `TARGET_COMM_PROFILE=uhf-primary`
- **THEN** they SHALL configure `COMM_CSP_NODE=6`
- **AND** the bounded proof SHALL first use the default target node-`5` path to switch active COMM to `UHF primary`
- **AND** only after that switch SHALL node-`6` ingress use `COMMAND_AUTHORITY_PROFILE=uhf-primary`

#### Scenario: UHF backup profile uses node 6 as backup ingress
- **WHEN** target launch, install, status, or probe surfaces use `TARGET_COMM_PROFILE=uhf-backup`
- **THEN** they SHALL configure `COMM_CSP_NODE=6`
- **AND** node-`6` ingress SHALL use `COMMAND_AUTHORITY_PROFILE=uhf-backup`

#### Scenario: Target UHF peer stays on subsystem.local serial ingress
- **WHEN** the target/lab UHF profile is installed and started
- **THEN** `subsystem.local` SHALL run `uhf_comm_csp_node` as CSP node `6`
- **AND** it SHALL own the configured `/dev/serial0` lab ingress plus the shared SocketCAN COMM carrier

### Requirement: Target Services Are Profile-Driven
The target/lab operator workflow SHALL expose an explicit target COMM profile instead of relying on implicit node defaults or independently free-form node/profile combinations.

#### Scenario: One OBC service entrypoint preserves default bootstrap semantics
- **WHEN** `obc-comm-csp-stack.service` starts
- **THEN** it SHALL bootstrap in the default target S-band configuration
- **AND** bounded node-`6` proofs SHALL layer their UHF authority/session behavior on top of that bootstrap baseline
- **AND** it SHALL NOT silently fall back to node `4`

#### Scenario: Subsystem services stay split by runtime boundary
- **WHEN** the subsystem target services are installed
- **THEN** EPS and ADCS SHALL remain separate services on `subsystem.local:can0`
- **AND** S-band node `5` and UHF node `6` SHALL each have their own COMM-facing service boundary on `subsystem.local:can1`
