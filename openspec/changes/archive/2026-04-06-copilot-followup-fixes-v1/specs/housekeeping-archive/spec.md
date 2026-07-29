## ADDED Requirements

### Requirement: Archive Record Version Changes With Incompatible Schema Growth
When the housekeeping archive record layout changes incompatibly, the repository-owned archive file header SHALL publish a new file-format version so readers and reviewers can distinguish the new record shape from prior files.

#### Scenario: Storage-health schema extension increments file version
- **WHEN** the housekeeping archive adds new serialized storage-health fields to each record
- **THEN** the archive file header SHALL use a newer file-format version than the prior pre-storage-health record layout
