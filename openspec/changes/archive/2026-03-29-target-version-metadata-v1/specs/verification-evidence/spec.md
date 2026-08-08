## ADDED Requirements

### Requirement: Raspberry Pi Version Metadata Evidence
The Raspberry Pi target evidence tree SHALL record the observed framework and project version metadata produced by the governed target build path, together with the commands used to generate or inspect that metadata.

#### Scenario: Target version metadata can be reviewed
- **WHEN** the target-version-metadata change completes
- **THEN** reviewers SHALL be able to inspect the target build commands and the resulting framework and project version outputs from the repository evidence tree
