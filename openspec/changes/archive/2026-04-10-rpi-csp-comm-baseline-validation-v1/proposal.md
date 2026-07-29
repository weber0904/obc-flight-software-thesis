## Why

The libcsp-first internal subsystem baseline has been validated on the hosted development path, while the repository also has governed Raspberry Pi external comm UART/RS485 paths. Before treating the libcsp base as the reliable future branch, the target-side evidence needs to show that Raspberry Pi execution can exercise both the internal CSP simulator network and the external comm UART stack without conflating those paths.

## What Changes

- Add a repo-owned Raspberry Pi probe that exercises internal CSP node reachability and external `/dev/serial0` comm behavior in one target run.
- Record evidence for Raspberry Pi OBC node `1`, EPS CSP node `2`, ADCS CSP node `3`, and the comm/radio/UART stack over the explicit target UART path.
- Reuse existing transparent/framed UART probes as optional stronger comm evidence when the serial adapter is available.
- Update docs and registry wording so internal CSP, external comm UART, and GDS remain distinct.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `platform-baseline`: Extend Raspberry Pi validation expectations to include the libcsp internal substrate after the migration.
- `comm-subsystem`: Treat Pi comm UART validation as part of the libcsp-base target readiness flow.
- `verification-evidence`: Record target-side CSP and comm evidence separately.
- `verification-path-registry`: Add a distinct registry entry for the combined Pi CSP + external comm baseline probe.

## Impact

- Affected scripts: new Raspberry Pi baseline probe wrapper.
- Affected docs/evidence: README, scripts README, verification matrix, registry, reconciliation matrix, and `evidence/records/rpi-csp-comm-baseline-validation-v1/`.
- Affected runtime behavior: none expected; the script drives existing target binaries and REPL commands.
