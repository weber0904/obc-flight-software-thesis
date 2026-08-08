## ADDED Requirements

### Requirement: Raspberry Pi Installable Bundle
The governed `integ-rpi` profile SHALL provide a repo-local packaging flow that emits a reviewable installable bundle for the Raspberry Pi target, and that bundle SHALL include the Linux OBC runtime, the companion simulator executables used by the integrated target flow, the deployment dictionary, and bundle metadata describing the packaged framework and project versions.

#### Scenario: Target bundle is created from governed artifacts
- **WHEN** the Raspberry Pi packaging flow runs after a successful target build
- **THEN** the repository SHALL produce a reviewable bundle artifact containing the installed-stack payload and bundle metadata without requiring the target to keep the full source workspace as the deployable unit

### Requirement: Raspberry Pi Installed Release Flow
The governed `integ-rpi` profile SHALL provide a repo-local install flow that can unpack a selected bundle into a fixed user-writable install root on the Raspberry Pi target, SHALL maintain a `current` release pointer, and SHALL support launching the integrated stack from that installed release path.

#### Scenario: Installed stack launches from current release
- **WHEN** an operator installs a governed Raspberry Pi bundle and launches the installed stack
- **THEN** the OBC runtime and simulator processes SHALL start from the installed release tree rather than from the synced source workspace
