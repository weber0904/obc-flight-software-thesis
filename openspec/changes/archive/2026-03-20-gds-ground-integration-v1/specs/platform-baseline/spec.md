## MODIFIED Requirements
### Requirement: Platform Integration Baseline
The project SHALL use F' v4.1.0 as the future bootstrap target, SHALL use ZMQ for internal CSP transport in the first version, SHALL document the default baseline ports `6100`, `7100`, `8080`, and `50000` plus the startup order of GDS, simulators, and OBC deployment, and SHALL provide hosted `dev-macos` entrypoints for both a local REPL stack and a GDS-connected stack without requiring Raspberry Pi hardware.

#### Scenario: Hosted dev-macos stack connects to GDS
- **WHEN** a developer launches the repository-local GDS-connected stack
- **THEN** the hosted OBC runtime SHALL connect to the `fprime-gds` IP adapter on the documented TCP port and SHALL not require a separate deployment-specific launcher outside the repository
