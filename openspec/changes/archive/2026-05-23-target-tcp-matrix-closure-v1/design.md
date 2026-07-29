## Context

The parity foundation change establishes the three-host target TCP topology and
closes command plus file cells. The remaining cells require higher-level
control flow:

- sequence upload plus execution plus subsystem round-trip
- S-band loss followed by UHF primary continuity

## Design

### Shared Inputs

This change reuses two previously-governed ingredients:

- the target TCP parity launcher
- the shared official sequencing helper

No parallel TCP-only sequence implementation is introduced.

### Remaining Target TCP Cells

The change closes:

- `sband-sequence-subsystem`
- `uhf-primary-sequence-subsystem`
- `failover-command`

Failover stays intentionally narrow: command continuity only, not file or
sequence closure inside the failover case itself.

## Boundaries

- No target direct-control case
- No new physical-UHF claim
- No change to the parity topology roles established by the prior change
