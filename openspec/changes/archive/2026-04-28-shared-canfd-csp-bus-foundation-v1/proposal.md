# shared-canfd-csp-bus-foundation-v1

## Why

The repository has already formalized the architecture direction that keeps GPS on direct OBC UART while moving CSP-facing subsystems toward a shared spacecraft-side CAN FD-capable bus. The current governed internal CSP carrier is still `zmqhub`, so the project cannot yet prove that the existing `EPS` and `ADCS` CSP contracts survive the first real physical-bus migration.

This change adds the first governed physical internal CSP carrier using Linux SocketCAN while keeping the current architecture boundaries intact:

- `GPS` remains `GY-GPS6MV2 -> obc.local:/dev/serial0 -> GpsBridge`
- `GDS -> OBC` remains the existing direct TCP ground path
- `EPS` and `ADCS` move from hosted `zmqhub` to a shared physical SocketCAN carrier
- `COMM` does not join this slice yet; its subsystem-side CAN channel is only reserved and self-tested

## What Changes

- Extend the libcsp runtime backend selection so Linux builds can bind node `1`, node `2`, and node `3` through `socketcan` as well as `zmqhub`.
- Add governed CAN-specific launchers for:
  - macOS ground-side GDS only
  - `subsystem.local` EPS/ADCS shared-bus hosting
  - `obc.local` OBC shared-bus hosting
- Add a governed three-host probe for:
  - `csp ping 2`
  - `csp ping 3`
  - `eps get`
  - `adcs get`
  - one bounded `fprime-cli -> GDS -> OBC -> EPS/ADCS` command path
- Record the first CAN FD-capable SocketCAN evidence with:
  - active-bus wiring
  - `parentdev` mapping
  - CAN statistics
  - active-bus `candump`
  - reserved-channel isolation evidence

## Impact

- Linux build prerequisites now include `libsocketcan-dev`; target bring-up docs also require `can-utils`.
- Existing hosted and split-host `zmqhub` baselines remain unchanged and continue to govern the software-only paths.
- This slice proves a physical internal CSP carrier, not COMM integration, omitted-RF TT&C, dual-bus redundancy, or independent EPS/ADCS physical controllers.
