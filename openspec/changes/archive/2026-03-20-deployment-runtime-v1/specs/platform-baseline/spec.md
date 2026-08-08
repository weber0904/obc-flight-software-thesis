## MODIFIED Requirements
### Requirement: Platform Integration Baseline
The project SHALL use F' v4.1.0 as the future bootstrap target, SHALL use ZMQ for internal CSP transport in the first version, SHALL document the default baseline ports `6100`, `7100`, and `8080` plus the startup order of proxy, simulators, OBC deployment, and GDS, and SHALL provide a hosted `dev-macos` runtime entrypoint that can launch the software-only OBC stack without requiring Raspberry Pi hardware.

#### Scenario: Hosted dev-macos stack starts locally
- **WHEN** a developer launches the repository-local software-only stack
- **THEN** the hosted OBC runtime SHALL start together with the default EPS simulator, ADCS simulator, and external comm mock path
