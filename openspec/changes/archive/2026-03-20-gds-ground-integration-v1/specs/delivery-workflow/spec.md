## MODIFIED Requirements
### Requirement: Follow-On Change Queue

The project SHALL maintain a named follow-on queue for `bootstrap-fprime-platform`, `core-system-contracts-v1`, `eps-subsystem-v1`, `adcs-subsystem-v1`, `comm-subsystem-v1`, `boot-update-v1`, `verification-ci-v1`, `deployment-runtime-v1`, and `gds-ground-integration-v1`, and the shared `resource-storage` capability SHALL be modified from those changes when needed rather than through a standalone implementation change.

#### Scenario: GDS integration change becomes part of delivery history
- **WHEN** the hosted stack gains a documented `fprime-gds` ground path
- **THEN** that work SHALL remain visible in the named change history and SHALL not be folded into earlier archived runtime work
