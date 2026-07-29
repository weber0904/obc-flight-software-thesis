## ADDED Requirements

### Requirement: Raspberry Pi Packaging Evidence
The Raspberry Pi evidence tree SHALL record the commands and observed results for target bundle creation, target installation into the governed install root, and installed-stack launch from that install root.

#### Scenario: Installed target flow is reviewable
- **WHEN** the Raspberry Pi packaging change completes
- **THEN** reviewers SHALL be able to inspect the bundle metadata, the install command path, and the installed-stack launch evidence from the repository documentation tree
