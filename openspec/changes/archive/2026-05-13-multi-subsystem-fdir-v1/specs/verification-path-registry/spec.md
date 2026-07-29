## ADDED Requirements

### Requirement: Hosted Multi-Subsystem Shared Recovery Path Is Registered Separately
The verification-path registry SHALL register the hosted bounded `EPS + ADCS + COMM` shared recovery path separately from the earlier watchdog-only and EPS-only recovery evidence.

#### Scenario: Registry names the hosted shared recovery path
- **WHEN** the hosted `multi-subsystem-fdir-v1` probe passes
- **THEN** the registry SHALL identify the newly proven path as `detector-local EPS/ADCS/COMM fault injection -> TopCcsds shared RecoveryExecutor -> bounded recovery action -> hosted reboot-equivalent relaunch truth`
- **AND** it SHALL cite the governing `docs/test-records/multi-subsystem-fdir-v1/README.md` evidence

#### Scenario: Registry keeps adjacent recovery paths distinct
- **WHEN** reviewers inspect the hosted multi-subsystem recovery entry
- **THEN** the registry SHALL keep earlier watchdog-v1, EPS-timeout-v1, and recovery-executors-v1 paths as separate adjacent proofs rather than treating any one of them as complete proof of the new three-subsystem closure

#### Scenario: Registry keeps broader FDIR claims out of scope
- **WHEN** the hosted multi-subsystem recovery path is registered
- **THEN** the entry SHALL state that it does not prove GPS, payload, TTC, storage-health, RF, target-hardware reboot, or a generic all-subsystem FDIR platform
