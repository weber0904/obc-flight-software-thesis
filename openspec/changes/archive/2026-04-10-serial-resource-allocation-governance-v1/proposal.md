## Why

GPS live UART cannot be scheduled as near-term work while the Raspberry Pi `/dev/serial0` device is already the governed external comm/radio UART path. Without a checked-in serial allocation rule, future changes may accidentally route GPS and comm through the same device and create an integration conflict.

## What Changes

- Reserve Raspberry Pi `/dev/serial0` for the external comm/radio path by default.
- Mark GPS live UART as blocked by unresolved serial allocation rather than as a generic missing-hardware task.
- Keep GPS fake/replay as the current validated GPS baseline.
- Document future options: USB-UART or a separately governed secondary-UART/pin-mux decision.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `comm-subsystem`: Owns the default Raspberry Pi `/dev/serial0` external comm allocation.
- `gps-subsystem`: Keeps live UART out of the validated baseline until a non-conflicting serial allocation is approved.
- `verification-evidence`: Requires constrained GPS live UART records to name the serial-resource conflict.
- `verification-path-registry`: Keeps GPS live UART separate from comm UART paths.

## Impact

- Affected docs/evidence: README, scripts README, verification matrix, registry, narrative docs, reporting package, and `evidence/records/serial-resource-allocation-governance-v1/`.
- Affected runtime code: none.
- Affected hardware planning: GPS live UART must not use `/dev/serial0` unless a future governed hardware architecture change reassigns that resource.
