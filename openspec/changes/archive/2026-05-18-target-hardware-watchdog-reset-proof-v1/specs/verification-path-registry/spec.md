## ADDED Requirements

### Requirement: Raspberry Pi Hardware Watchdog Reset Path Is Registered Separately

The verification-path registry SHALL add a distinct Raspberry Pi hardware
watchdog reset entry once
`target-hardware-watchdog-reset-proof-v1` records passing evidence, and that
entry SHALL remain separate from hosted watchdog supervision, target
service-managed `R2` restart, generic Linux reboot, and power-loss recovery.

#### Scenario: Hardware watchdog registry entry stays distinct from service restart
- **WHEN** the Raspberry Pi hardware watchdog reset path is registered
- **THEN** the registry SHALL identify the active `obc-comm-csp-stack.service`
  target path, governing evidence, watchdog-source trigger boundary, and reboot
  readback contract
- **AND** it SHALL keep target `R2` process restart as a neighboring but
  distinct verification path

#### Scenario: Hardware watchdog registry entry stays distinct from Linux reboot
- **WHEN** the Raspberry Pi hardware watchdog reset path is registered
- **THEN** the registry SHALL state that the path proves board reset caused by
  the hardware watchdog timeout under the active OBC baseline
- **AND** it SHALL NOT collapse that path into generic Linux reboot,
  bootloader/partition handoff, or power-loss recovery claims
