## ADDED Requirements

### Requirement: Target Service-Managed R2 Recovery Restart
The governed `integ-rpi` profile SHALL allow the active service-managed OBC release to restart after a `RecoveryExecutor` R2 process-restart request through the same systemd-managed installed or lab service model that owns target startup.

#### Scenario: Systemd restarts OBC after R2 process exit
- **WHEN** the active OBC process exits with the bounded R2 process-restart exit code under the governed Raspberry Pi service
- **THEN** the owning launch script SHALL return a nonzero status to systemd
- **AND** systemd SHALL restart the OBC service through the configured `Restart=on-failure` policy
- **AND** the relaunched process SHALL come from the active `TopCcsds` `OBC` release path rather than the retired legacy topology

#### Scenario: Target R2 restart does not claim hardware reset
- **WHEN** the target service-managed R2 restart path is documented or cited as evidence
- **THEN** the evidence SHALL identify the path as managed OBC process/service restart
- **AND** it SHALL NOT claim Raspberry Pi hardware watchdog reset, Linux reboot, bootloader or partition handoff, power-loss recovery, RF behavior, or final flight deployment behavior
