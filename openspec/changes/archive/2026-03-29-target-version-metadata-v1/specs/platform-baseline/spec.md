## ADDED Requirements

### Requirement: Raspberry Pi Target Version Metadata
The governed `integ-rpi` build flow SHALL generate correct framework and project version metadata for the Raspberry Pi target even when the synced target workspace omits `.git`, and the target build path SHALL use host-derived version inputs instead of falling back to the framework default release string.

#### Scenario: Synced target workspace builds without git metadata
- **WHEN** the Raspberry Pi bootstrap flow builds the project from a synced workspace that excludes `.git`
- **THEN** the generated version metadata SHALL report the intended framework and project versions rather than the fallback `v3.5.0`
