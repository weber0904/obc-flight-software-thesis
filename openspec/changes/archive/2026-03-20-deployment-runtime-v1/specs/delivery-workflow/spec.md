## MODIFIED Requirements
### Requirement: Follow-On Change Queue

The project SHALL maintain a named follow-on queue for `bootstrap-fprime-platform`, `core-system-contracts-v1`, `eps-subsystem-v1`, `adcs-subsystem-v1`, `comm-subsystem-v1`, `boot-update-v1`, `verification-ci-v1`, and `deployment-runtime-v1`, and the shared `resource-storage` capability SHALL be modified from those changes when needed rather than through a standalone implementation change.

#### Scenario: Runnable integration change becomes part of delivery history
- **WHEN** the project closes the gap between isolated capability slices and a runnable hosted OBC stack
- **THEN** that work SHALL be tracked as a named archived change rather than treated as undocumented cleanup
