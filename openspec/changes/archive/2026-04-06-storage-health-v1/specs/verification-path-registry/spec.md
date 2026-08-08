## ADDED Requirements

### Requirement: Storage Health Hosted Validation Path Is Registered
The repository verification-path registry SHALL include a dedicated entry for the first hosted storage health validation path and SHALL describe that path as distinct from any still-unimplemented Raspberry Pi target disk-health or cleanup-policy path.

#### Scenario: Hosted storage path can be reused without implying cleanup or target coverage
- **WHEN** a later change wants to reuse the first storage health validation baseline
- **THEN** reviewers SHALL be able to cite one registry entry that names the hosted governed-root storage path, its governing evidence, and the still-out-of-scope target-disk or cleanup-policy work
