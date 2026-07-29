## ADDED Requirements

### Requirement: Legacy Top Retirement Is Registered As A Cleanup Boundary
The verification-path registry SHALL identify the retired `OBC/Top` /
`OBC_ComFprimeLegacy` path as historical-only after
`legacy-top-retirement-docs-reorg-v1`, and SHALL keep active CCSDS hosted and
target paths distinct from historical ComFprime evidence.

#### Scenario: Registry distinguishes active and historical OBC ground paths
- **WHEN** reviewers inspect registry entries that mention stock `ComFprime`,
  old UHF backup, or old S-band gateway evidence
- **THEN** the registry SHALL state whether the entry is historical-only or an
  active reusable path
- **AND** the registry SHALL NOT treat historical `OBC_ComFprimeLegacy` evidence
  as proof of the maintained active `OBC` / `TopCcsds` path.

#### Scenario: Current helper paths cite active topology sources
- **WHEN** registry or verification inventory helpers describe current
  maintained source surfaces
- **THEN** they SHALL refer to active `OBC/TopCcsds` sources instead of
  `OBC/Top` sources.

## MODIFIED Requirements

### Requirement: R2 Process Restart Paths Are Registered
The repository verification-path registry SHALL include entries for the hosted
and Raspberry Pi service-managed R2 process restart paths once
`target-recovery-closure-v1` records passing evidence, and those entries SHALL
remain distinct from R6 reboot-equivalent, Linux reboot, hardware watchdog
reset, power-loss paths, and historical legacy Top evidence.

#### Scenario: Hosted R2 registry entry stays distinct from R6
- **WHEN** the hosted R2 process-restart path is registered
- **THEN** the registry SHALL identify the exact hosted runtime path, governing
  evidence, R2 exit-code behavior, and boot metadata readback
- **AND** it SHALL keep R6 reboot-equivalent closure as a neighboring but
  distinct path.

#### Scenario: Target R2 registry entry stays distinct from hardware reset
- **WHEN** the Raspberry Pi service-managed R2 process-restart path is
  registered
- **THEN** the registry SHALL identify the service-managed OBC target path, the
  induced ADCS R2 fault boundary, systemd restart observation, governing
  evidence, and boot metadata readback
- **AND** it SHALL state that hardware watchdog reset, Linux reboot, bootloader
  or partition handoff, power-loss recovery, and historical legacy Top behavior
  remain unproven by that entry.
