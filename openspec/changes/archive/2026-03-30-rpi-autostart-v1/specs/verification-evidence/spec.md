## ADDED Requirements

### Requirement: Raspberry Pi Autostart Evidence
The Raspberry Pi evidence tree SHALL record the commands and observed results for governed service installation, service status inspection, target reboot, and post-reboot launch from the installed release.

#### Scenario: Autostart flow is reviewable after reboot
- **WHEN** the Raspberry Pi autostart change completes
- **THEN** reviewers SHALL be able to inspect the service install path, the reboot probe, the post-reboot service status, and the observed installed-release startup evidence from the repository documentation tree
