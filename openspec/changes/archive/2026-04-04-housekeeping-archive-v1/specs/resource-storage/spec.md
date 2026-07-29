## ADDED Requirements

### Requirement: Housekeeping Archive Runtime Roots
The runtime storage model SHALL place housekeeping archive files and the housekeeping archive index under a governed mutable runtime-root path that remains outside installed release payload directories.

#### Scenario: Release switching preserves housekeeping history
- **WHEN** the governed runtime launches from a hosted or Raspberry Pi runtime root
- **THEN** the housekeeping archive files and index SHALL live under the shared mutable runtime area rather than inside a versioned release payload
