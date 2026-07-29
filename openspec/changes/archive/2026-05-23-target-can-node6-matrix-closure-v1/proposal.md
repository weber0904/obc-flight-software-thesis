## Why

The current target CAN matrix still leaves several node-`6` or UHF-related
cells blocked, even though the repository already has adjacent governed target
proofs for quiet UHF, target restart recovery, and physical UART southbound.
The missing work is not a new target architecture; it is a case-granularity
closure problem.

This change closes the target CAN cells that can and should be proven on the
current quiet-UHF lab baseline.

## What Changes

- Reuse `run_rpi_target_recovery_restart_probe.sh` as the target CAN command
  path backbone and extract shared helper logic where needed.
- Close target CAN `sband-sequence-subsystem`, `uhf-primary-file`,
  `uhf-primary-sequence-subsystem`, and `failover-command`.
- Keep quiet-UHF acceptance truthful and avoid any non-quiet claim.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `verification-evidence`: require reviewable target CAN matrix proof for the
  remaining node-`6` and failover capability cells on the current quiet-UHF
  baseline.

## Impact

- Affected code:
  - target CAN case wrappers
  - shared target helper logic extracted from the current governed target probe
  - focused target CAN evidence and umbrella dashboard updates
- Public/operator impact:
  - more target CAN matrix cells move from blocked to governed evidence
- Non-goals:
  - no non-quiet UHF baseline expansion
  - no target TCP parity work
