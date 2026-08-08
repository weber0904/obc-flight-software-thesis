## MODIFIED Requirements

### Requirement: Platform Integration Baseline
The project SHALL use F' v4.1.0 as the future bootstrap target, SHALL use `libcsp` as the internal subsystem network substrate, SHALL allow the hosted `dev-macos` profile to bind that internal network through libcsp's official ZMQHUB-backed transport, SHALL document the default baseline ports `6100`, `7100`, `8080`, and `50000` plus the startup order of the CSP hub or proxy, GDS, simulators, and OBC deployment, SHALL provide hosted `dev-macos` entrypoints for both a local REPL stack and a GDS-connected stack without requiring Raspberry Pi hardware, and SHALL provide repo-local `integ-rpi` entrypoints that can build and launch the same governed workspace on a Raspberry Pi through configuration, portable artifact discovery, and target-specific runtime paths instead of a separate unmanaged deployment tree.

#### Scenario: Near-term multi-host topology uses named roles
- **WHEN** the repository documents or configures the near-term multi-host topology
- **THEN** it SHALL describe `macOS` as the ground host, `obc` / `obc.local` as the OBC target host, and `subsystem-sim` / `subsystem.local` as the subsystem simulator host instead of treating one generic Raspberry Pi hostname as the only governed default

#### Scenario: OBC-target scripts keep backward compatibility while adopting role-based naming
- **WHEN** an existing Raspberry Pi helper script targets the OBC host
- **THEN** it SHALL resolve the OBC target through a canonical `OBC_SSH_TARGET` setting, SHALL continue to accept `RPI_SSH_TARGET` as a compatibility alias, and SHALL fall back to the governed default `operator@obc.local` when neither variable is provided

#### Scenario: Subsystem simulator host has a formal configuration surface
- **WHEN** a future change needs to target the subsystem-side Raspberry Pi host
- **THEN** the platform baseline SHALL already provide a formal `SUBSYSTEM_SIM_SSH_TARGET` configuration name whose default value is `operator@subsystem.local`, even if no full subsystem-host orchestration workflow has been added yet
