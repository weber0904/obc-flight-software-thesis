## ADDED Requirements

### Requirement: Narrative And Formal Baselines
The project SHALL maintain a two-layer documentation model consisting of narrative source documents in `obc-dev-spec/` and formal main specifications in `openspec/specs/`. When the two layers diverge, the formal main specifications SHALL take precedence and the narrative layer SHALL be reconciled in a later change.

#### Scenario: Formal baseline overrides narrative
- **WHEN** a narrative source statement conflicts with an archived formal main spec
- **THEN** future implementation and validation work SHALL follow the formal main spec

### Requirement: Supported Profiles And Modes
The platform SHALL define the `dev-macos`, `integ-rpi`, and `flight-hw-future` profiles, and profile switching SHALL be expressed through configuration, instance selection, endpoints, or device paths rather than duplicating business logic.

#### Scenario: Profile switch preserves core behavior
- **WHEN** the deployment changes from `dev-macos` to `integ-rpi`
- **THEN** the transport and endpoint configuration MAY change while the core application logic SHALL remain shared

### Requirement: Platform Integration Baseline
The project SHALL use F' v4.1.0 as the future bootstrap target, SHALL use ZMQ for internal CSP transport in the first version, and SHALL document the default baseline ports `6100`, `7100`, and `8080` plus the startup order of proxy, simulators, OBC deployment, and GDS.

#### Scenario: Baseline environment startup
- **WHEN** an operator prepares the baseline software-only environment
- **THEN** the documented startup order and default ports SHALL be sufficient to bring up proxy, simulators, deployment, and GDS
