## Why

Existing gateway-backed COMM evidence uses the generic `comm_csp_node` identity as node `4`. The next COMM work needs explicit S-band and UHF simulator process identities without making old node-4 evidence ambiguous or claiming the full future ground paths too early.

## What Changes

- Preserve `comm_csp_node` as generic compatibility COMM on node `4`.
- Add explicit `sband_comm_csp_node` and `uhf_comm_csp_node` executable identities with default nodes `5` and `6`.
- Reuse the existing `CommNodeServer`, `CommSimModel`, and COMM CSP services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS` for all three identities.
- Add startup/log/probe surfaces that keep generic, S-band, and UHF link identity distinct.
- Add hosted foundation evidence proving the three process identities can coexist on the governed internal CSP substrate and respond on the unchanged COMM service contract.
- Keep complete S-band GDS path, UHF UART backup path, CCSDS, RF behavior, reliable transfer, file/downlink, and Pi hardware out of scope.

## Capabilities

### New Capabilities

- none

### Modified Capabilities

- `comm-subsystem`: define the dual-link COMM simulator foundation identities and compatibility boundary.
- `verification-evidence`: require bounded evidence for the hosted dual-link COMM simulator foundation.
- `verification-path-registry`: register the hosted dual-link COMM simulator identity/coexistence path as distinct from full S-band/UHF link validation.

## Impact

- Affected code:
  - `simulators/comm/` for shared COMM node main support and thin executable entrypoints
  - `simulators/CMakeLists.txt` for new executable targets and focused probe helper
  - `scripts/` for the hosted foundation probe
- Affected interfaces:
  - new executable names and default node constants
  - no change to existing node `4`, service ports `30-32`, packet layouts, `GroundLinkDriver`, `ComFprime`, or GDS gateway semantics
- Affected docs/evidence:
  - simulator/script README updates
  - new OpenSpec deltas, test record, and verification-path registry entry
