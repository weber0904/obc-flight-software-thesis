## Overview

`reliable-transfer-v1` adds the first bounded reliable-transfer slice for the
current default S-band node-`5` path without broadening the COMM baseline. The
selected transfer family is the current official HK `.fdp` family produced
through `DpCatalog`. The transport unit is a fixed-size file segment inside one
transfer context, and the owner remains `CommController`.

This design is intentionally narrower than a generic transport redesign:

- one active transfer at a time
- current default S-band node-`5` only
- official `.fdp` requests only
- current whole-command retries remain outside transfer truth
- no restart-persistent resume
- no UHF, dual-link, RF, or CFDP platform claim

## Current Baseline Behavior

### Ownership and path

- `DpCatalog` owns HK `.fdp` generation, catalog build, and transmit selection.
- `CommController` owns shared downlink admission and active owner truth.
- The current default path is:
  `DpCatalog -> CommController -> FileDownlink -> COMM DOWNLINK_WRITE -> node 5 -> ground_ttc_gateway -> GDS file store`

### Current success/failure truth

- `FileDownlink` packetizes files with `Fw::FilePacket`.
- The current proof surface is bounded file/downlink parity, not bounded
  reliable transfer.
- If a transfer attempt is interrupted, current operational recovery is a new
  whole-command retry from the ground side.
- Current baseline does not distinguish in-transfer timeout/resend semantics
  from operator retries.

### Baseline preserved by this change

- `DpCatalog` still generates and selects official HK `.fdp` files.
- `CommController` still owns admission, busy reject, owner drop, and primary
  file-link policy.
- `SESSION_OPEN(seq0)`, command authority, gateway role, and observability
  ownership stay unchanged.
- Stock `FileDownlink` remains present as the existing baseline/fallback path
  and is not deleted or repurposed into a generic reliable-transfer engine.

## First-Version Reliable-Transfer Behavior

### Transfer unit

The v1 transfer unit is:

- one whole-file transfer context for one official `.fdp` artifact
- internally partitioned into fixed-size file segments
- bounded by a cumulative-ACK resend window

This change does not implement packet-level generic retry for every COMM
service and does not implement arbitrary resume after process restart.

### Owner and protocol boundary

- `CommController` remains the policy owner for admission and active transfer
  ownership.
- A new bounded helper inside `CommController` owns transfer execution:
  - read selected source file
  - emit `START/DATA/END/CANCEL` aligned payloads
  - maintain send window state
  - poll cumulative ACK state
  - resend after bounded no-progress timeout
  - finalize success or failure
- `DpCatalog` remains the upstream initiator and completion consumer.
- The new reliable-transfer data plane does not reuse stock `FileDownlink` as
  the active sender for selected v1 transfers.

### Path selection

`CommController` routes a request into the v1 reliable path only when all of
the following are true:

- primary file link is current S-band
- the source file name ends with `.fdp`
- the request is a whole-file request (`offset == 0`, `length == 0`)

All other requests stay on the current baseline path through stock
`sendFileOut -> FileDownlink`.

### Protocol shape

The v1 reliable path adds a narrow COMM sidecar service family on node `5`:

- port `37`: reliable-transfer data frames
- port `38`: reliable-transfer control (`BEGIN`, `ACK_POLL`, `COMPLETE`,
  `CANCEL`, `ABORT`)

The existing ports `30/31/32/33` remain current baseline services and are not
redefined as reliable-transfer semantics.

### FilePacket alignment

The v1 sender and receiver use `Fw::FilePacket` vocabulary:

- `BEGIN` carries a serialized `START` packet plus SHA-256 metadata
- data frames carry serialized `DATA` packets
- `COMPLETE` carries a serialized `END` packet
- `CANCEL` and `ABORT` carry serialized `CANCEL` packets

This keeps v1 aligned with current F´ file-delivery vocabulary without claiming
full CFDP compatibility.

### Receiver

The node-`5` ground-side COMM receiver becomes repo-owned transfer truth for
this path:

- create one temp file per transfer context
- accept one active transfer at a time
- write unique segments at the specified offsets
- ignore duplicate segments within the active transfer context
- track highest contiguous ACK boundary
- verify final size, CFDP checksum, and SHA-256 before promotion
- only promote the temp file to a final artifact path after verification

### Fixed v1 limits

- segment payload ceiling: `160` bytes
- cumulative ACK window size: `2` segments
- ACK/no-progress timeout: `1000 ms` equivalent to one transfer tick
- resend budget: `3` window resends without forward progress
- transfer ceiling: `64` segments / `10,240` bytes per file
- active transfers: `1`
- restart-persistent resume: `none`

### Completion truth

V1 success requires all of the following:

1. `BEGIN` accepted
2. all `DATA` segments reach the receiver and advance the cumulative ACK
   boundary
3. `COMPLETE` accepted
4. receiver final verification passes for file size, CFDP checksum, and
   SHA-256
5. only then does `CommController` clear the active owner with success and
   notify `DpCatalog`

Ground-side whole-command retries are not part of this success definition.

### Failure Semantics

| Case | Sender behavior | Receiver behavior | Final truth |
|---|---|---|---|
| Success | send `BEGIN`, all `DATA`, `COMPLETE`; observe cumulative ACK progress | promote temp file only after size + checksum + SHA-256 pass | success |
| Timeout | ACK boundary does not advance within one timeout interval | temp file may contain partial data | sender resends current outstanding window |
| Bounded retry exhausted | resend budget reaches `3` with no forward progress | report current contiguous boundary; delete temp file on `ABORT` | final failure |
| Duplicate within transfer context | resend may retransmit an already received segment | ignore duplicate payload and preserve contiguous ACK boundary | not a failure by itself |
| Cancel | local owner drop or explicit cancel | delete temp file; preserve bounded progress report in reply | cancelled |
| Abort | sender aborts after bounded failure | delete temp file; preserve bounded progress report in reply | aborted/final failure |
| Partial progress with final failure | some ACK progress was achieved before failure | return highest contiguous segment and bytes before cleanup | failure with partial progress evidence |

### Interaction with current owner/policy boundaries

- `CommController` still decides whether a request is admitted.
- The helper never claims command authority ownership.
- The helper never changes `SESSION_OPEN(seq0)` rules.
- The helper never changes gateway role.
- `DpCatalog` still sees one accepted whole-file request and one final
  completion callback.

## Future Broader Transport / CFDP / UHF / Dual-Link Work

This change explicitly leaves future work for:

- UHF reliable-transfer semantics
- simultaneous dual-link or relay arbitration
- RF/over-the-air closure
- restart-persistent resume
- broader artifact-family coverage
- generic all-path MTU governance
- full CFDP-style adoption across more links and more products

Any follow-up that expands beyond the local v1 ceilings must land as a new
change rather than being silently implied by this one.
