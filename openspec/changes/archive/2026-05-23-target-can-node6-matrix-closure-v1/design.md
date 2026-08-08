## Context

The current target CAN baseline already has:

- node `5` default S-band
- node `6` bounded quiet UHF
- physical UHF UART southbound
- shared SocketCAN interior

The remaining matrix gaps are concentrated in sequence, file, and failover
proof granularity rather than missing infrastructure.

## Design

### Shared Target Helper

The change extracts reusable target helper logic from
`run_rpi_target_recovery_restart_probe.sh` for:

- authority and session bootstrap
- optional UHF primary switch prerequisites
- ground-path setup and teardown
- bounded physical-UART preflight discipline

### Target CAN Cell Closure

The change closes these cells:

- `sband-sequence-subsystem`
- `uhf-primary-file`
- `uhf-primary-sequence-subsystem`
- `failover-command`

Each UHF-related case keeps quiet-aware acceptance. Ground-side command or file
evidence remains mandatory; journal-only passes are not allowed.

## Boundaries

- No new target/lab topology
- No direct-control case yet
- No non-quiet background telemetry claim
