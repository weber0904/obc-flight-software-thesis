## ADDED Requirements

### Requirement: Active OBC Raspberry Pi Alignment Evidence
The verification evidence tree SHALL record reviewable Raspberry Pi package,
install, and autostart evidence that the governed target-facing release path now
ships the active `OBC` / `TopCcsds` payload rather than a renamed legacy
deployment.

#### Scenario: Package evidence proves active payload truth
- **WHEN** `active-topccsds-target-alignment-v1` records final package evidence
- **THEN** reviewers SHALL be able to inspect the package command, manifest, and
  bundle contents showing that `bin/OBC` and
  `dict/AppTopologyDictionary.json` were sourced from the active `OBC`
  deployment
- **AND** the evidence SHALL keep any still-legacy lab-only helpers explicit
  instead of implying they are part of the active package truth.

#### Scenario: Installed and rebooted target evidence stays on the active path
- **WHEN** `active-topccsds-target-alignment-v1` records installed-release and
  autostart evidence
- **THEN** reviewers SHALL be able to inspect the installed current-release
  launch result and the post-reboot service result on the corrected active
  package path
- **AND** the evidence SHALL describe that proof as target package/startup
  alignment rather than as full COMM lab-operational closure.
