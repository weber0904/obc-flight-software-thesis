## Why

The repository already has a bounded GPS subsystem with fake/replay support, cached runtime state, and hosted evidence, but it still lacks the first governed hardware-backed GPS ingest path. The project has now chosen to connect `GY-GPS6MV2` directly to `obc.local` and to move `/dev/serial0` from the old OBC-side external comm baseline to GPS. Without a dedicated follow-on slice, future hardware work would either bypass the existing `GpsBridge` contract or silently conflict with the prior serial-allocation assumptions.

This change is needed now to add a real UART-backed GPS source while preserving the existing fake/replay baselines and keeping the first live hardware scope intentionally narrow.

## What Changes

- Extend the GPS source model from `fake/replay` to `fake/replay/live-uart`
- Add governed GPS UART runtime configuration through `OBC_GPS_SOURCE_MODE`, `OBC_GPS_SERIAL_DEVICE`, and `OBC_GPS_BAUDRATE`
- Implement a dedicated line-oriented UART NMEA source for `GpsBridge` without routing GPS through `comm-subsystem`
- Add focused L1 serial-source tests plus classic `GpsBridge` component-harness coverage for `live-uart`
- Add a target-side `run_rpi_gps_live_probe.sh` that proves real hardware sentence ingestion on `obc.local:/dev/serial0`
- Realign specs and evidence so `/dev/serial0` is now the active GPS UART path and the old OBC-side serial comm baseline becomes historical evidence rather than the current target baseline

## Capabilities

### New Capabilities
- none

### Modified Capabilities
- `gps-subsystem`: add first live hardware UART source mode while keeping fake/replay as the software baseline
- `comm-subsystem`: mark the old OBC-side `/dev/serial0` UART path as historical/superseded for the active target baseline
- `platform-baseline`: record direct OBC-attached GPS UART as the active near-term target GPS path
- `verification-evidence`: require reviewable hardware GPS evidence and explicit separation from historical serial comm evidence
- `verification-path-registry`: register the new live GPS UART path and prevent reuse of the old comm-UART proof as GPS proof

## Impact

- Affected code:
  - `simulators/gps/` source support
  - `OBC/Components/GpsBridge/`
  - `OBC/Main.cpp`
  - new target probe script under `scripts/`
- Affected APIs:
  - `GpsSourceMode` adds `LIVE_UART`
  - `gps source <...>` operator surface adds `live-uart`
  - new governed GPS UART env vars
- Affected systems:
  - hosted GPS regression remains valid
  - Raspberry Pi target serial ownership moves to GPS for the active baseline
