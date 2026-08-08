## Why

The active hosted `OBC` baseline already uses `TopCcsds` for the default S-band path, but its UHF node-`6` path still traverses the local `OBCComFprime` subtopology. That leaves the active topology split across two communication generations: CCSDS for S-band and stock `ComFprime` framing for UHF. This makes UHF behavior a structural legacy dependency inside the active topology even though `Top` and `OBC_ComFprimeLegacy` are now explicit fallback or regression paths.

This change upgrades the active UHF path to a repo-local CCSDS path so the active hosted `OBC` no longer depends on `OBCComFprime`. It keeps `Top` and `OBC_ComFprimeLegacy` intact for historical regression and future retirement work.

## What Changes

- Add a repo-local `OBCComCcsds` UHF subtopology beside `TopCcsds`.
- Remove active `TopCcsds` wiring to `OBCComFprime`.
- Rewire the full active UHF surface to the local CCSDS UHF path:
  - driver send and receive
  - rate-group scheduling
  - command ingress and response
  - packet egress
  - file and downlink egress
- Keep UHF as a separate CCSDS stack instance rather than sharing the S-band `ComCcsds` instance.
- Keep shared packet semantics with S-band:
  - `SCID = 0x44`
  - APIDs `0/1/2/3`
- Make UHF link identity explicit with `VCID = 2`.
- Add active hosted UHF CCSDS evidence and update active docs and registry truth.
- Keep `run_uhf_uart_backup_link_probe.sh`, `Top`, and `OBC_ComFprimeLegacy` as historical `ComFprime` regression surfaces in this wave.

## Capabilities

### New Capabilities

- Active hosted UHF CCSDS node-`6` path with decoded framing evidence.

### Modified Capabilities

- `comm-subsystem`: active UHF path is CCSDS-backed instead of `ComFprime`-backed.
- `verification-evidence`: adds independent UHF CCSDS hosted adoption evidence while preserving historical `ComFprime` UHF evidence.
- `verification-path-registry`: adds a reusable active UHF CCSDS path and narrows the historical UHF `ComFprime` path to regression status.

## Impact

- `OBC/TopCcsds` topology wiring, local subtopology glue, and runtime setup.
- Hosted UHF probes and active COMM session/downlink evidence.
- Current architecture truth, verification-path registry, and OpenSpec capability wording for active UHF.
