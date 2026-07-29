## ADDED Requirements

### Requirement: Three-Host Split-Host CSP Topology Is Governed
The platform baseline SHALL allow a governed three-host topology where `macOS` runs the shared CSP hub and headless `fprime-gds`, `obc.local` runs only the OBC process, and `subsystem.local` runs only EPS node `2` plus ADCS node `3`.

#### Scenario: Host responsibilities stay explicit in the three-host topology
- **WHEN** the repository launches the governed three-host split-host topology
- **THEN** `macOS` SHALL host `csp_zmqproxy` plus GDS, `subsystem.local` SHALL host the subsystem simulator processes, and `obc.local` SHALL host only the OBC process instead of collapsing those roles back into one remote host

### Requirement: Subsystem Simulator Host Uses A Governed Workspace Flow
The platform baseline SHALL provide a governed workspace-based prep flow for `subsystem.local` using `SUBSYSTEM_SIM_SSH_TARGET` and `SUBSYSTEM_SIM_REMOTE_DIR`, and that flow SHALL remain distinct from the installed-bundle or systemd-managed OBC lifecycle.

#### Scenario: Subsystem simulator host builds from a synced workspace
- **WHEN** a developer prepares `subsystem.local` for the governed three-host topology
- **THEN** the repository SHALL be able to sync the workspace and build the simulator binaries on that host without requiring an installed bundle or autostart service there
