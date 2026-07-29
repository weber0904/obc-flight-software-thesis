## Why

The current physical lab serial COMM evidence proves bounded command ingress, command events, and telemetry, but it explicitly leaves file/downlink behavior out of scope. This change closes that next verification gap without broadening the COMM wire contract or introducing arbitrary file access.

## What Changes

- Add a hosted PTY COMM file/downlink regression probe that uses stock `fprime-gds`, existing COMM CSP services, and the existing housekeeping archive downlink commands.
- Add a strict physical lab serial COMM file/downlink probe that requires command/event/channel TT&C first, then validates housekeeping index and multiple slot files through `FileDownlink`.
- Generate at least two occupied housekeeping archive slots via paced `HK_CAPTURE_NOW` commands over the same GDS/COMM path.
- Downlink `HK_DOWNLINK_INDEX` and two `HK_DOWNLINK_SLOT` files, then compare the received ground files byte-for-byte against the OBC runtime source files.
- Preserve COMM node `4`, services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`; no new COMM service ports, wire layouts, public flight commands, or generic arbitrary-file command are added.
- Keep RF, target OBC migration, no-preamble first-byte-clean behavior, ScenarioBridge/pass automation, and COMM shared CAN FD participation out of scope.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `comm-subsystem`: add the bounded COMM file/downlink proof boundary using existing housekeeping archive commands over the COMM TT&C path.
- `ground-ttc-gateway`: require gateway-backed COMM file/downlink claims to include received ground files and byte-for-byte source comparison.
- `verification-evidence`: require file/downlink evidence to record endpoints, slots, received files, source comparisons, and excluded adjacent paths.
- `verification-path-registry`: register physical lab serial COMM file/downlink separately from bounded command/event/channel TT&C after the formal probe passes.

## Impact

- Adds repository-owned probe scripts under `scripts/`.
- Adds an evidence record under `docs/test-records/comm-ttc-file-downlink-v1/`.
- Updates the COMM roadmap, verification-path registry, specs, and reconciliation matrix after archive.
- Reuses existing `HousekeepingArchive` and F' `FileDownlink`; no product behavior or wire-shape change is intended beyond probe orchestration and governance records.
