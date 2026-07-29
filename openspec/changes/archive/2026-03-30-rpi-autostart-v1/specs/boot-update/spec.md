## ADDED Requirements

### Requirement: Governed Post-Reboot Startup Path
The first Raspberry Pi boot integration slice SHALL provide a governed post-reboot startup path that relaunches the installed `current` release after the target OS boots, and that path SHALL keep boot/update state observable through the existing public boot contract without claiming firmware- or partition-level handoff support.

#### Scenario: System reboot relaunches the installed current release
- **WHEN** the Raspberry Pi target reboots after the governed autostart service has been installed and enabled
- **THEN** the installed `current` release SHALL come back through the governed service path and SHALL continue to expose `BOOT_STATUS` state from file-backed metadata
