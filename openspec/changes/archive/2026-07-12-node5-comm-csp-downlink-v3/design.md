## Context

The maintained official node-`5` S-band path still relies on stock
`FileDownlink` behavior at the owner boundary. A single stock file packet can
now be bounded to fit inside one `2032`-byte `v3` data frame, but the
repo-local COMM CSP transport still needs to stop re-splitting those buffers
into smaller CSP transactions. The `v2` uplift
already introduced node-`5` staging/commit/drain decoupling, duplicate
handling, and bounded queue-backed acceptance semantics. The remaining major
cost is sender-side stop-and-wait, not receiver-side drain latency.

## Goals / Non-Goals

**Goals**
- Keep `DpCatalog -> CommController -> FileDownlink -> CommEgressMux ->
  GroundLinkDriver` unchanged.
- Add a node-`5`-only bulk downlink `v3` that uses one-way data frames plus
  control polling below `GroundLinkDriver`.
- Make `GroundLinkDriver.send()` succeed once the entire upper-layer buffer has
  been committed into node-`5` memory, not when the external link flushes.
- Avoid reusing the existing whole-file preload reliable-transfer sender.
- Keep ground-side `ground_ttc_gateway -> fprime-gds` unchanged.

**Non-Goals**
- No payload-family reliable-transfer widening.
- No ground-side CSP participant, new gateway protocol, or end-to-end reliable
  delivery.
- No node-`6` migration.
- No CAN FD conversion of EPS/ADCS traffic and no BRS/data-phase-rate claim.
- No `ComQueue` priority redesign in this slice.

## Decisions

### V3 uses RT-shaped control, not RT implementation

`v3` borrows the good parts of the existing reliable-transfer family:
one-way data, bounded control polling, resend-from-progress, and explicit
complete/abort. It does **not** reuse the current `CommReliableTransfer`
component or its whole-file preload model.

### Generic bytes below GroundLinkDriver

`v3` operates on the serialized buffer passed into `GroundLinkDriver.send()`.
The node-`5` receiver stores generic byte frames rather than decoding or
assuming `Fw::FilePacket::DATA`.

### One control port, one data port

`DOWNLINK_CONTROL_V3` carries `BEGIN`, `ACK_POLL`, `COMMIT`, `ABORT`, and
`STATUS` using a single request/reply shape. `DOWNLINK_DATA_V3` carries one-way
raw data frames through `sendRaw()`.

### Keep v2 in tree, but not in official selection

The backend probes `v3` only on node `5`. If unavailable, it falls directly to
`v1`. Existing `v2` code remains for reference, regression comparison, and
artifact continuity, but the official runtime path becomes `v3 -> v1`.

### Bounded packet-pool increase

The repo-local CSP runtime packet pool moves to `CSP_BUFFER_SIZE=2048` and
`CSP_BUFFER_COUNT=24`, while `FW_FILE_BUFFER_MAX_SIZE` moves to `2032`. This
raises the static pool footprint on OBC and node-`5`, but keeps it bounded
while allowing one stock serialized file buffer to fit inside one
`DOWNLINK_DATA_V3` frame. This slice does not widen pool depth beyond `24`.

### Fixed initial window

The sender uses a fixed initial window of `3` frames. This is large enough to
break the stop-and-wait bottleneck while remaining simple to test and reason
about. Window auto-tuning is deferred.

### Scoped CAN FD is part of the governed target profile

The `2032`-byte V3 data packet requires a scoped SocketCAN CAN FD carrier on
the maintained physical target path. The baseline manager (`A`) owns and
verifies `COMM_CSP_SOCKETCAN_USE_CANFD=1`, destination allowlist `5,6`, and
data-port allowlist `40` for the OBC plus S-band/UHF COMM services. The
functional probe (`C`) verifies that effective environment and does not
create, restart, or remove the shared profile.

The scope is deliberately narrow: EPS/ADCS services do not receive this
drop-in and their traffic remains classical CAN. The libcsp change selects CAN
FD frame format for the allowlisted V3 traffic but does not set `CANFD_BRS`, so
the proof must not be described as BRS or confirmed 2 Mbit/s data-phase
closure.

## Protocol Shape

- `DOWNLINK_CONTROL_V3` request fields:
  - `RequestHeader` with version `3`
  - `op`
  - `streamId`
  - `totalFrames`
  - `totalBytes`
  - `committedFrames`
  - `committedBytes`
- `DOWNLINK_CONTROL_V3` reply fields:
  - `ReplyHeader`
  - `op`
  - `streamId`
  - `contiguousFrames`
  - `contiguousBytes`
  - `windowCredit`
  - `drainQueuedFrames`
  - `drainFreeFrames`
  - `stagingActive`
  - `stagingStreamId`
  - `acceptedBytes`
  - `flushedBytes`
  - `droppedCommittedBytes`
  - `duplicateFrames`
- `DOWNLINK_DATA_V3` frame fields:
  - `version`
  - `kind`
  - `streamId`
  - `frameIndex`
  - `frameCount`
  - `byteOffset`
  - `byteCount`
  - `data[2032]`

## Risks / Trade-offs

- [Static libcsp pool cost increases on both OBC and node-`5`] -> keep the new
  size bounded at `2048/24` and avoid increasing pool depth in the same slice.
- [Telemetry queue overflow may remain or worsen under faster file send] ->
  explicitly record hosted/target residual behavior in proof evidence, but do
  not redesign queue priorities in this slice.
- [Target OBC can still serialize node-`5` bulk traffic behind unrelated CSP
  work] -> current OBC runtime ownership remains a single-worker queue, so
  governed target evidence must explicitly check for head-of-line blocking or
  timeout coupling that does not appear in hosted proofs.
- [A large CSP packet fails when fragmented into classical CAN frames on the
  maintained target carrier] -> keep CAN FD scoped to destinations `5,6` and
  data port `40`, and make A verify it before C starts.
- [Commit retry could duplicate bytes] -> make `BEGIN` and `COMMIT`
  idempotent, and keep duplicate data frames from mutating staged bytes.
- [Mid-stream failure could leave stale state] -> use explicit `ABORT` plus a
  `2000 ms` staging timeout as cleanup insurance.

## Migration Plan

1. Add `v3` protocol structs, runtime sizing, and backend/server support in
   parallel with the existing `v1/v2` code.
2. Switch official node-`5` backend selection to `v3 -> v1`.
3. Extend unit tests and focused hosted proof coverage.
4. Promote the scoped COMM CAN FD condition from a probe-owned overlay to the
   governed target A-layer baseline.
5. Requalify full hosted and governed target official `.fdp` paths against the
   new transport.
