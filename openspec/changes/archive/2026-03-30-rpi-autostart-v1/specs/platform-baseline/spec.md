## ADDED Requirements

### Requirement: Raspberry Pi Service-Managed Startup
The governed `integ-rpi` profile SHALL provide a repo-local autostart flow that installs a systemd-managed service for the installed `current` release on the Raspberry Pi target, and that service SHALL launch the governed stack from the install root rather than from the synced source workspace.

#### Scenario: Target service starts the installed current release
- **WHEN** an operator installs and enables the governed Raspberry Pi autostart service
- **THEN** the target boot path SHALL start the OBC runtime and companion simulator stack from `$OBC_HOME/obc-deploy/current` or the configured install-root equivalent instead of the development workspace

### Requirement: Headless Installed Runtime Mode
The governed Raspberry Pi startup path SHALL support a headless runtime mode so the OBC process can run under systemd without depending on interactive stdin input, while the existing interactive launch path remains available for manual operator sessions.

#### Scenario: Service-managed runtime does not require REPL input
- **WHEN** the installed stack runs under the governed systemd service
- **THEN** the OBC runtime SHALL remain active without waiting for or failing on interactive stdin input
