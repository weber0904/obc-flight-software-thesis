## Why

The completed dual-link simulator foundation introduced `sband_comm_csp_node` as the S-band COMM simulator identity on CSP node `5`, but it intentionally did not prove a S-band ground path. The next formal COMM step needs a governed S-band simulated TCP segment that still traverses COMM and stock `ComFprime`, rather than reusing the adjacent direct `GDS -> TCP -> OBC` development baseline as evidence.

## What Changes

- Add S-band TCP external-link support to the hosted COMM simulator and ground gateway path.
- Use `sband_comm_csp_node` as the spacecraft-side S-band endpoint, default CSP node `5`.
- Make `sband_comm_csp_node` listen on a bounded TCP endpoint while `ground_ttc_gateway` connects to that S-band simulated RF segment.
- Keep OBC in `GROUND_LINK_MODE=comm-csp` with `COMM_CSP_NODE=5`, so stock `ComFprime` remains the GDS-facing protocol while the transport traverses COMM services `30-32`.
- Add a repository-owned hosted probe that proves bounded command/event/channel TT&C and bounded housekeeping archive file/downlink byte-match over the S-band TCP COMM path.
- Preserve generic `comm_csp_node` node `4` compatibility and `uhf_comm_csp_node` node `6` foundation behavior.

## Capabilities

### New Capabilities

- none

### Modified Capabilities

- `comm-subsystem`: define the S-band TCP COMM path through node `5` without changing the COMM CSP service contract.
- `ground-ttc-gateway`: allow the gateway southbound segment to use S-band simulated TCP while keeping stock F' northbound framing.
- `verification-evidence`: require bounded TT&C and file/downlink evidence for the S-band TCP through-COMM path.
- `verification-path-registry`: register distinct hosted S-band TCP TT&C and hosted S-band TCP file/downlink paths.

## Impact

- Affected code:
  - `simulators/comm/` for TCP external-link support in the COMM node and gateway
  - `scripts/` for the hosted S-band TCP probe
- Affected interfaces:
  - add TCP endpoint CLI options to `sband_comm_csp_node`/shared COMM node app support and `ground_ttc_gateway`
  - no change to service ports `30-32`, COMM packet layouts, node `4`, node `6`, `ComFprime`, or stock `fprime-gds`
- Affected docs/evidence:
  - simulator/script README updates
  - new OpenSpec deltas, test record, verification-path registry entries, and reconciliation matrix entry

## Scope Boundary

This change proves only hosted S-band simulated TCP through COMM for bounded command/event/channel traffic and bounded housekeeping archive file/downlink. It does not claim UHF UART backup, CCSDS, RF behavior, reliable transfer, packet-loss recovery, target hardware, Raspberry Pi deployment, pass scheduling, arbitrary onboard file downlink, or custom GDS plugin behavior.
