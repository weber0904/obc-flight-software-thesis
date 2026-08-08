## Why

The completed dual-link simulator foundation introduced `uhf_comm_csp_node` as the UHF COMM simulator identity on CSP node `6`, but it intentionally proved only identity, coexistence, and services `30-32`. The completed S-band TCP change proved a separate S-band node `5` ground-link path, but that evidence cannot stand in for the UHF UART/RS485/USB/macOS backup path.

This change creates the next UHF-specific proof boundary: hosted serial backup command ingress and hosted link-local beacon emission through the UHF simulator identity.

## What Changes

- Use `uhf_comm_csp_node` as the spacecraft-side UHF endpoint, default CSP node `6`.
- Prove bounded backup command ingress over a hosted PTY serial segment that stands in for the macOS UART/USB/RS485 lab path.
- Keep OBC in `GROUND_LINK_MODE=comm-csp` with `COMM_CSP_NODE=6`, so stock `ComFprime` remains the GDS-facing protocol while command ingress traverses COMM services `30-32`.
- Add a UHF node-6 beacon side channel so BeaconV1 frames can be emitted through `uhf_comm_csp_node` to a hosted serial capture path without changing the command/downlink chunk service contract.
- Add a repository-owned hosted probe that proves bounded command/event/channel ingress and captures/decodes at least one BeaconV1 frame from the UHF node-6 beacon path.
- Preserve generic `comm_csp_node` node `4` compatibility and `sband_comm_csp_node` node `5` S-band TCP behavior.

## Capabilities

### New Capabilities

- none

### Modified Capabilities

- `comm-subsystem`: define the UHF hosted UART backup path through node `6`, plus a bounded node-6 beacon side channel.
- `ground-ttc-gateway`: identify the UHF serial southbound segment separately from S-band TCP and direct GDS TCP.
- `verification-evidence`: require reviewable bounded command ingress and BeaconV1 capture/decode evidence for UHF v1.
- `verification-path-registry`: register distinct hosted UHF serial backup TT&C ingress and UHF node-6 beacon paths.

## Impact

- Affected code:
  - `simulators/comm/` for the UHF beacon side-channel endpoint and any shared COMM node support needed to keep link identities explicit
  - `OBC/` runtime/topology wiring only as needed to send BeaconV1 frames toward the UHF node-6 side channel
  - `scripts/` for the hosted UHF UART backup/beacon probe
- Affected interfaces:
  - no change to `comm_csp_node` node `4`, `sband_comm_csp_node` node `5`, command/downlink services `30-32`, existing packet layouts, `ComFprime`, stock `fprime-gds`, or S-band TCP launcher behavior
  - add only bounded UHF beacon-side-channel configuration and logs needed for reviewable evidence
- Affected docs/evidence:
  - simulator/script README updates
  - new OpenSpec deltas, test record, verification-path registry entries, and reconciliation matrix entry

## Scope Boundary

This change proves only hosted UHF UART/RS485/USB/macOS stand-in behavior using PTY serial segments: bounded backup command ingress through `uhf_comm_csp_node(node 6)` and bounded BeaconV1 emission through a UHF node-6 beacon side channel. It does not claim S-band TCP, direct `GDS -> TCP -> OBC`, UHF full command authority, failover policy, reliable transfer, arbitrary file downlink, CCSDS, RF behavior, target hardware, Raspberry Pi deployment, or physical USB/RS485 electrical behavior.
