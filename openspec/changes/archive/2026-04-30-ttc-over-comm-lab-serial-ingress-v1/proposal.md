## Why

The hosted PTY-backed COMM TT&C path is proven, and the physical subsystem UART wiring now has separate macOS-initiated request/reply and subsystem-origin acquisition evidence. The next step is to replace the hosted PTY stand-in with the real macOS-to-`subsystem.local` serial ingress without prematurely treating a full TT&C failure as a downlink-only problem.

## What Changes

- Add a staged lab serial ingress probe for `ground_ttc_gateway -> physical UART -> subsystem.local comm_csp_node -> CSP -> hosted OBC`.
- Make uplink command ingress to OBC readback the minimum formal pass boundary.
- Attempt `fprime-cli events` and `fprime-cli channels` downlink after uplink succeeds, but record downlink failure as diagnostic rather than TT&C success.
- Add an explicit, opt-in gateway serial TX preamble for the physical lab probe because current UART diagnostics show the link is not clean-first-frame reliable without acquisition/settling.
- Allow bounded command retries in the staged physical probe while still requiring final EPS and ADCS OBC readback before Stage 1 passes.
- Preserve stock F' framing, stock `fprime-gds`, native-built `subsystem.local` workflow, COMM node `4`, and services `30` through `32`.
- Keep RF, real radio behavior, file/downlink, target OBC migration, scenario link availability, and COMM shared CAN FD out of scope.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: require a staged physical lab serial ingress proof that distinguishes uplink ingress from downlink/full TT&C.
- `ground-ttc-gateway`: allow the physical lab serial gateway validation to close on uplink ingress when downlink remains diagnostic.
- `verification-evidence`: require evidence to name Stage 1 uplink ingress and Stage 2 downlink/full-TT&C verdicts separately.
- `verification-path-registry`: register any proven physical lab serial ingress path separately from hosted PTY TT&C, UART preflight, and lab acquisition paths.

## Impact

- Adds a repository-owned staged probe under `scripts/`.
- Adds opt-in physical serial acquisition preamble controls to `ground_ttc_gateway`.
- Adds a bounded evidence record under `docs/test-records/ttc-over-comm-lab-serial-ingress-v1/`.
- May register a new verification path only after the staged probe proves its formal boundary.
- Does not change the COMM CSP wire contract, `ground_ttc_gateway` northbound interface, `fprime-gds`, subsystem build strategy, or OBC target deployment model.
