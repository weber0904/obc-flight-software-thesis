## ADDED Requirements

### Requirement: Target Node-5 Migration Evidence Is Reviewable
The verification evidence tree SHALL record reviewable target/lab evidence showing that node `5` replaced generic node `4` as the default target COMM path.

#### Scenario: Evidence records target node-5 default proof
- **WHEN** target node-`5` migration evidence is recorded
- **THEN** the evidence SHALL include the target profile used, subsystem node-`5` service identity, target OBC node-`5` runtime configuration, command/readback observations, and final verdict

### Requirement: Target Node-6 Quiet UHF Evidence Is Reviewable
The verification evidence tree SHALL record reviewable target/lab evidence for node `6` under both `uhf-primary` and `uhf-backup` bounded quiet-mode proofs.

#### Scenario: Evidence records both node-6 role variants
- **WHEN** target node-`6` migration evidence is recorded
- **THEN** the evidence SHALL identify separate bounded results for `uhf-primary` and `uhf-backup`
- **AND** it SHALL record that quiet mode was probe-owned and not a new nominal operator baseline
- **AND** it SHALL record that `uhf-primary` evidence used an explicit node-`5` switch-to-UHF step before node-`6` command/readback

### Requirement: Target Reboot-Class Evidence Follows Node 5 After Migration
The verification evidence tree SHALL update target reboot-class records so the migrated default node-`5` path is the governing COMM path for those proofs.

#### Scenario: Recovery and watchdog evidence cite node 5
- **WHEN** target `R2` recovery restart or target hardware-watchdog reset evidence is refreshed after migration
- **THEN** the evidence SHALL identify the default node-`5` target path as the governing COMM path for that proof
- **AND** it SHALL keep any separate quiet-mode node-`6` operational evidence as adjacent rather than as the reboot-class baseline
