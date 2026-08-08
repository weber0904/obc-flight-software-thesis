## ADDED Requirements

### Requirement: Target Recovery Closure Evidence
The verification evidence tree SHALL record reviewable hosted and Raspberry Pi evidence for `target-recovery-closure-v1`, including focused unit coverage, hosted R2/R6 distinction, service-managed target R2 restart proof, OpenSpec validation, and explicit remaining hardware-reset boundaries.

#### Scenario: Focused tests cover R2 restart semantics
- **WHEN** `target-recovery-closure-v1` completes local verification
- **THEN** the evidence SHALL identify tests covering R2 metadata persistence, process-restart pending/count state, R2 versus R6 exit-code separation, boot metadata reload after R2, and repeated-recovery clamp behavior

#### Scenario: Hosted probes prove R2 and R6 distinction
- **WHEN** hosted recovery probes are recorded for this change
- **THEN** the evidence SHALL include the probe command, isolated runtime roots or ports, expected R2 process-restart exit observations, expected R6 reboot-equivalent observations, post-relaunch boot metadata observations, and final verdict

#### Scenario: Raspberry Pi target probe proves service-managed R2 restart
- **WHEN** target evidence is recorded for this change
- **THEN** the evidence SHALL include the service-managed target path, the ADCS subsystem-service stop method used to induce the R2 fault, OBC service restart observations, restored ADCS service state, command-path recovery observation, boot metadata readback, and final verdict

#### Scenario: Evidence keeps hardware reset out of scope
- **WHEN** `target-recovery-closure-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove Raspberry Pi hardware watchdog reset, Linux reboot, bootloader or partition handoff, power-loss recovery, RF behavior, or final flight deployment behavior
