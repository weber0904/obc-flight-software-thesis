## Why

The repository now proves command/event/channel TT&C through COMM node `4` on `subsystem.local:can1` to target OBC on `obc.local:can0`, but it explicitly leaves file/downlink over that SocketCAN-backed COMM path unproven. The next bounded step is to prove the existing housekeeping archive file/downlink command surface across the same physical path without adding new commands, service ports, wire layouts, or RF claims.

## What Changes

- Add a formal physical SocketCAN-backed COMM file/downlink probe that reuses the proven COMM SocketCAN TT&C startup path.
- Add a diagnostic TT&C-only mode for the same file/downlink startup shape so failures before HK/file commands can be classified before formal file assertions.
- Require TT&C readiness before file assertions: OBC readback, command events, `GROUND_LINK_TX_BYTES`, active CAN captures, and CAN health remain part of the prerequisite.
- Downlink `hk-index.csv` and two occupied `hk-slot-*.bin` files through existing `HK_DOWNLINK_INDEX` and `HK_DOWNLINK_SLOT`.
- Snapshot source files from the target OBC runtime tree immediately before each downlink and byte-compare received GDS files against those snapshots.
- Record evidence and register the new path separately from physical lab serial file/downlink and from SocketCAN command/event/channel TT&C.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: register bounded housekeeping archive file/downlink over the existing SocketCAN-backed COMM path.
- `ground-ttc-gateway`: define gateway-backed file/downlink validation when COMM forwards to target OBC over SocketCAN.
- `verification-evidence`: require evidence to record source snapshots, received files, byte comparisons, CAN health, and excluded adjacent paths.
- `verification-path-registry`: add a distinct path for physical COMM SocketCAN housekeeping archive file/downlink.

## Impact

- Adds one repository-owned physical probe script.
- Adds one evidence record under `docs/test-records/comm-csp-socketcan-file-downlink-v1/`.
- Updates the registry, roadmap, and reconciliation surfaces after archive.
- Does not add a flight command, arbitrary-file downlink command, COMM CSP service port, custom GDS plugin, RF behavior, no-preamble claim, ScenarioBridge/pass automation, or dual-bus redundancy.
