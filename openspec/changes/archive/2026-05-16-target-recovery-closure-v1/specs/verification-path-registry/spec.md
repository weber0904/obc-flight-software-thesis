## ADDED Requirements

### Requirement: R2 Process Restart Paths Are Registered
The repository verification-path registry SHALL include entries for the hosted and Raspberry Pi service-managed R2 process restart paths once `target-recovery-closure-v1` records passing evidence, and those entries SHALL remain distinct from R6 reboot-equivalent, Linux reboot, hardware watchdog reset, and power-loss paths.

#### Scenario: Hosted R2 registry entry stays distinct from R6
- **WHEN** the hosted R2 process-restart path is registered
- **THEN** the registry SHALL identify the exact hosted runtime path, governing evidence, R2 exit-code behavior, and boot metadata readback
- **AND** it SHALL keep R6 reboot-equivalent closure as a neighboring but distinct path

#### Scenario: Target R2 registry entry stays distinct from hardware reset
- **WHEN** the Raspberry Pi service-managed R2 process-restart path is registered
- **THEN** the registry SHALL identify the service-managed OBC target path, the induced ADCS R2 fault boundary, systemd restart observation, governing evidence, and boot metadata readback
- **AND** it SHALL state that hardware watchdog reset, Linux reboot, bootloader or partition handoff, and power-loss recovery remain unproven by that entry
