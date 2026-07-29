## ADDED Requirements

### Requirement: RecoveryExecutor Executes R2 Process Restart
The mission-autonomy capability SHALL make `RecoveryExecutor` execute a real managed OBC process restart for current `R2_RESTART_SOFTWARE_COMPONENT` recovery sources instead of stopping at process-restart intent reporting.

#### Scenario: Current R2 sources request managed process restart
- **WHEN** a watchdog-source stale fault or an ADCS scheduled-poll transport or freshness fault enters shared recovery at `R2_RESTART_SOFTWARE_COMPONENT`
- **THEN** `RecoveryExecutor` SHALL persist recovery metadata for that source and level before requesting runtime exit
- **AND** it SHALL mark the active recovery action as `PROCESS_RESTART`
- **AND** it SHALL NOT mark that R2 action as `OBC_REBOOT`

#### Scenario: Runtime exit distinguishes R2 from R6
- **WHEN** the active runtime consumes a pending `PROCESS_RESTART` request from `RecoveryExecutor`
- **THEN** the runtime SHALL exit with the bounded process-restart exit code
- **AND** R6 reboot-equivalent requests SHALL continue to use the existing reboot-equivalent exit code

#### Scenario: Repeated-recovery clamp prevents R2 restart loops
- **WHEN** boot metadata reports that boot-after-recovery safe fallback is required
- **AND** a current R2 source opens or relatches a shared recovery incident
- **THEN** `RecoveryExecutor` SHALL record the incident and hold or request the safe fallback path
- **AND** it SHALL NOT queue another R2 process restart before the runtime has been acknowledged stable

#### Scenario: R2 state is reported separately from reboot state
- **WHEN** `RecoveryExecutor` reports runtime recovery status
- **THEN** process-restart pending and process-restart count SHALL be observable separately from reboot pending and reboot count
- **AND** R2 process restart SHALL NOT increment the R6 reboot counter
