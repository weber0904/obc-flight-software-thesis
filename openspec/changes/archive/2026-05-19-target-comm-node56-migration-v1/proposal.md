## Why

Target/lab COMM still treats generic node `4` as the active operator baseline even though hosted COMM identity has already converged on S-band node `5` and UHF node `6`. That split keeps target proofs, runbooks, and service graphs anchored to a compatibility path instead of the intended link identities.

## What Changes

- Retire active target/lab use of generic COMM node `4` and migrate target/lab to S-band node `5` plus UHF node `6`.
- Add explicit target communication profiles so target services and probes select `sband`, `uhf-primary`, or `uhf-backup` instead of free-form node/profile combinations.
- Add target-side subsystem services for `sband_comm_csp_node` and `uhf_comm_csp_node`, and update OBC target launch/install/status surfaces to derive runtime config from the selected profile.
- Move existing target reboot-class proofs to the new default node-`5` path.
- Add bounded quiet-mode target proofs for node-`6` `uhf-backup` and `uhf-primary-after-switch`.
- Update canonical target docs, registry entries, and operator runbooks so node `4` remains historical/compatibility-only instead of active target truth.

## Capabilities

### New Capabilities

- `target-comm-node56-migration`: Formal target/lab migration from generic node `4` to default S-band node `5` plus selectable UHF node `6`.

### Modified Capabilities

- `comm-subsystem`: Target/lab COMM identities, services, and operational baselines change from generic node `4` to target node `5/6` profiles.
- `verification-path-registry`: Active target/lab path registration changes to node `5` default plus bounded quiet-mode node `6` proofs.
- `verification-evidence`: Target evidence requirements expand to record node-`5` default proof and node-`6` quiet-mode proofs after migration.

## Impact

- Target/lab launch, install, status, and probe scripts
- `subsystem.local` systemd service templates and target OBC systemd/runtime configuration
- Target operator runbook, active architecture truth, verification registry, and change evidence
