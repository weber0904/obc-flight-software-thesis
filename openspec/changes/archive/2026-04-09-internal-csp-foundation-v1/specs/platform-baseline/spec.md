## MODIFIED Requirements

### Requirement: Platform Integration Baseline

The project SHALL use F' v4.1.0 as the future bootstrap target, SHALL use `libcsp` as the internal subsystem network substrate, SHALL allow the hosted `dev-macos` profile to bind that internal network through libcsp's official ZMQHUB-backed transport, SHALL document the default baseline ports `6100`, `7100`, `8080`, and `50000` plus the startup order of the CSP hub/proxy, GDS, simulators, and OBC deployment, SHALL provide hosted `dev-macos` entrypoints for both a local REPL stack and a GDS-connected stack without requiring Raspberry Pi hardware, and SHALL provide repo-local `integ-rpi` entrypoints that can build and launch the same governed workspace on a Raspberry Pi through configuration, portable artifact discovery, and target-specific runtime paths instead of a separate unmanaged deployment tree.

#### Scenario: Hosted internal CSP substrate uses official ZMQHUB semantics
- **WHEN** the hosted development profile starts the internal subsystem network substrate
- **THEN** it SHALL use a repo-local hub/proxy process plus libcsp's official ZMQHUB-backed interface semantics instead of a project-local direct ZMQ request/response transport

#### Scenario: Ground and internal network settings remain distinct
- **WHEN** the hosted stack is configured for both GDS connectivity and internal CSP connectivity
- **THEN** the configuration SHALL keep GDS adapter settings distinct from internal CSP hub settings and SHALL not describe them as one transport layer
