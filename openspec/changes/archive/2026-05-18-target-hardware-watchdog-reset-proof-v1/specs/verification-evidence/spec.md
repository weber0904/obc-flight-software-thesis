## ADDED Requirements

### Requirement: Target Hardware Watchdog Reset Evidence

The verification evidence tree SHALL record reviewable capability-gate,
integration, and Raspberry Pi target evidence for
`target-hardware-watchdog-reset-proof-v1`, including proof that the active OBC
service owns the hardware watchdog device, that watchdog-source stale
suppression leads to board reboot, and that existing `R2` process restart truth
remains intact.

#### Scenario: Capability gate records lifecycle compatibility
- **WHEN** `target-hardware-watchdog-reset-proof-v1` completes its capability
  gate
- **THEN** the evidence SHALL record watchdog presence, driver identity, fixed
  timeout behavior, clean-stop observation, reopen observation, and any
  constraints discovered for the active baseline

#### Scenario: Target probe proves hardware watchdog reset
- **WHEN** Raspberry Pi target evidence is recorded for this change
- **THEN** the evidence SHALL include the active service path, the bounded proof
  trigger, the observation that watchdog stroking stopped, SSH disconnect and
  reconnect observations, reboot evidence, service recovery, boot metadata
  readback, persistent fault readback, and final verdict
- **AND** if the proof uses a temporary quiet diagnostic path, the evidence
  SHALL record that quiet mode was probe-owned, that journal-first acceptance
  was used, and that the service was restored to normal non-quiet mode before
  the probe exited

#### Scenario: Evidence proves R2 did not regress
- **WHEN** this change records target watchdog-reset evidence
- **THEN** it SHALL also record a rerun of the existing target `R2`
  process-restart proof or an equivalent focused regression result
- **AND** that evidence SHALL confirm `R2` still uses service-managed restart
  rather than hardware watchdog reset

#### Scenario: Evidence keeps power-loss and external supervisor out of scope
- **WHEN** `target-hardware-watchdog-reset-proof-v1` evidence is recorded
- **THEN** it SHALL state that the evidence does not prove power-loss recovery,
  external supervisor IC behavior, bootloader or partition handoff, secure boot,
  RF behavior, or final flight deployment behavior
