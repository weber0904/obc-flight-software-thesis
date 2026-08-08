## Why

The current node-`5` COMM CSP downlink path still preserves official
`DpCatalog -> CommController -> FileDownlink` ownership, but its repo-local
transport remains dominated by synchronous per-chunk request/reply behavior.
The existing `v2` uplift regularized node-`5` staging and drain semantics, yet
it still requires the sender to wait on every chunk-level reply and therefore
does not materially improve official `.fdp` delivery time on the maintained
path.

## What Changes

- Add a node-`5`-only COMM CSP bulk downlink `v3` below `GroundLinkDriver`
  that uses one-way data frames plus bounded control polling/commit/abort.
- Keep stock `DpCatalog -> CommController -> FileDownlink -> CommEgressMux ->
  GroundLinkDriver` ownership unchanged.
- Keep `v1` `DOWNLINK_WRITE` as the runtime fallback; retain `v2` in tree only
  for historical/reference coverage, not as official fallback.
- Raise the repo-local CSP runtime packet pool sizing to `2048` bytes / `24`
  buffers, reduce `FW_FILE_BUFFER_MAX_SIZE` to `2032`, and let `v3` carry a
  full stock `GroundLinkDriver.send()` file buffer in one `DOWNLINK_DATA_V3`
  frame instead of re-splitting it into smaller CSP transactions.
- Make the governed target A-layer own the scoped SocketCAN CAN FD profile
  required by the `2032`-byte node-`5`/`6` COMM carrier: destinations `5,6`,
  data port `40`. EPS/ADCS remain on classical CAN traffic.
- Extend hosted and governed target proof paths, unit tests, and verification
  records to cover the new `v3` transport and its residual telemetry-queue
  interaction explicitly.

## Capabilities

### New Capabilities
- `node5-comm-csp-downlink-v3`: node-`5`-only bulk downlink transport below
  `FileDownlink` using one-way data frames, windowed progress polling, and
  atomic commit-to-drain acceptance.

### Modified Capabilities
- `comm-subsystem`: node-`5` S-band COMM service exposure and downlink
  transport behavior gain a `v3` bulk path and `v3 -> v1` fallback rule.
- `verification-path-registry`: maintained node-`5` hosted and target official
  `.fdp` paths must record `v3` timing/acceptance behavior without widening
  reliable-transfer or ground-side protocol scope.

## Impact

- Affected code: forked libcsp SocketCAN framing and runtime sizing,
  `CommCspProtocol`,
  `GroundLinkBackend`, `CommNodeServer`, hosted proof tools, and COMM unit
  tests.
- Affected systems: node-`5` S-band COMM service, OBC-side COMM CSP runtime,
  hosted official `.fdp` path, and governed target node-`5` payload proof.
- Public behavior: stock file/downlink ownership remains unchanged; only the
  node-`5` transport below `GroundLinkDriver` changes.
- Carrier claim: this change proves scoped Linux SocketCAN CAN FD framing for
  the COMM V3 data service. It does not claim BRS, a measured 2 Mbit/s data
  phase, RF/OTA closure, or CAN FD migration of EPS/ADCS.
