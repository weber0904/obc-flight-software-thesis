## ADDED Requirements

### Requirement: Target Node-5 Default Path Is Registered Separately
The verification-path registry SHALL register the migrated target/lab default path as a node-`5` S-band COMM path distinct from the older target node-`4` compatibility path.

#### Scenario: Registry names the target node-5 default boundary
- **WHEN** target node-`5` default-path evidence passes
- **THEN** the registry SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> TCP -> subsystem.local sband_comm_csp_node(node 5) -> shared SocketCAN -> obc.local OBC`
- **AND** it SHALL describe the older target node-`4` path as historical or compatibility-only rather than as current target baseline

### Requirement: Target Node-6 Quiet UHF Paths Are Registered Separately
The verification-path registry SHALL register target node-`6` `uhf-primary` and `uhf-backup` command/readback proofs as bounded quiet-mode paths distinct from both the default target node-`5` path and hosted UHF evidence.

#### Scenario: Registry names the quiet node-6 boundary
- **WHEN** target node-`6` quiet-mode evidence passes
- **THEN** the registry SHALL identify the path as `fprime-cli -> fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> physical serial -> subsystem.local uhf_comm_csp_node(node 6) -> shared SocketCAN -> obc.local OBC`
- **AND** it SHALL record whether the bounded proof used `uhf-primary` or `uhf-backup`
- **AND** it SHALL record that `uhf-primary` is proven only after explicit switch from the default target node-`5` path

#### Scenario: Registry keeps node-6 proof scope bounded
- **WHEN** target node-`6` quiet-mode paths are registered
- **THEN** the entry SHALL state that the path does not prove general non-quiet serial/background-TM stability, simultaneous dual-link runtime, or beacon/handshake/retry policy

### Requirement: Target Reboot-Class Proofs Follow Node 5
The verification-path registry SHALL record existing target recovery-restart and hardware-watchdog proofs under the migrated default node-`5` target path rather than under a remaining node-`4` or node-`6` baseline.

#### Scenario: Registry binds recovery proofs to node 5
- **WHEN** target reboot-class evidence is updated after migration
- **THEN** the registry SHALL identify target `R2` recovery restart and target hardware-watchdog reset as proofs exercised over the default target node-`5` COMM path
- **AND** it SHALL keep quiet-mode node-`6` operational proof as a separate adjacent path
