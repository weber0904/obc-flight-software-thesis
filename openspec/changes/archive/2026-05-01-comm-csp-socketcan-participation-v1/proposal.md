## Why

The repository has a governed physical SocketCAN baseline for EPS/ADCS, and it has a separate physical lab serial COMM TT&C baseline. Those proofs deliberately leave COMM shared CAN FD participation out of scope. The next architecture step is to prove that COMM node `4` can join the spacecraft-side SocketCAN bus while preserving the existing COMM TT&C service contract and the existing EPS/ADCS CAN path.

The first attempt at planning this change exposed an important validation rule: a hardware CAN smoke must bring the CAN interfaces `UP` with explicit timing before judging physical connectivity. This change makes that rule part of the repo-owned probes instead of relying on prior operator state.

## What Changes

- Add a corrected Stage 0 EPS/ADCS SocketCAN health smoke that brings up `obc.local:can0` and `subsystem.local:can0` before checking `csp ping 2`, `csp ping 3`, `eps get`, and `adcs get`.
- Add target and subsystem launch support for OBC `comm-csp` ground-link mode over SocketCAN while EPS/ADCS remain on `subsystem.local:can0` and COMM node `4` uses `subsystem.local:can1`.
- Add a formal physical COMM SocketCAN TT&C probe that:
  - starts headless GDS and `ground_ttc_gateway` on macOS
  - starts EPS/ADCS on subsystem CAN `can0`
  - starts `comm_csp_node` on subsystem CAN `can1` with existing lab serial ingress
  - starts target OBC on `obc.local:can0` with `GROUND_LINK_MODE=comm-csp`
  - proves command readback, command events, and `GROUND_LINK_TX_BYTES` telemetry through the path
- Record evidence that `subsystem.local:can1` changes from reserved-channel status to active COMM participation on the shared CAN FD-capable bus for this proof.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: register COMM node `4` SocketCAN participation with existing services `30`, `31`, and `32`.
- `platform-baseline`: update the near-term CAN role allocation from reserved `can1` to active COMM participation for this change.
- `ground-ttc-gateway`: define the gateway-backed TT&C validation path when COMM reaches OBC over SocketCAN instead of hosted ZMQHUB.
- `verification-path-registry`: add a distinct path for physical COMM SocketCAN command/event/channel TT&C.
- `verification-evidence`: require evidence to record CAN bring-up, interface health, CAN captures, TT&C observations, and excluded adjacent paths.

## Impact

- Adds repository-owned scripts for CAN bring-up and COMM SocketCAN TT&C validation.
- Adds one evidence record under `evidence/records/comm-csp-socketcan-participation-v1/`.
- Does not change COMM CSP service ports, wire layouts, F' command definitions, RF behavior, file/downlink behavior, or no-preamble serial behavior.
