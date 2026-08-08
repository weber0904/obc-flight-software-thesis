## Context

The prior COMM SocketCAN TT&C change proved this bounded path:

```text
GDS -> ground_ttc_gateway -> lab serial ingress
  -> COMM node 4 on subsystem.local:can1
  -> shared SocketCAN bus
  -> target OBC on obc.local:can0
  -> COMM downlink -> GDS
```

The existing physical lab serial file/downlink proof already shows that housekeeping archive files can traverse the COMM TT&C service contract in hosted-ZMQHUB mode. This change combines those two established boundaries and proves the missing file/downlink behavior when the internal COMM-to-OBC carrier is SocketCAN.

## Decisions

- Reuse the existing command surface only.
  - `HK_CAPTURE_NOW` creates occupied housekeeping archive slots.
  - `HK_DOWNLINK_INDEX` downlinks `hk-index.csv`.
  - `HK_DOWNLINK_SLOT` downlinks selected occupied slot files.
- Reuse the formal SocketCAN TT&C setup.
  - Bring up `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1` before judging connectivity.
  - Keep EPS/ADCS on `subsystem.local:can0`; keep COMM node `4` on `subsystem.local:can1`.
  - Keep COMM services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`.
- Keep the file transfer bounded for the constrained physical carrier.
  - Use a project-local `FW_FILE_BUFFER_MAX_SIZE` of `256` bytes so stock F' `FileDownlink` data packets stay below the generic COM buffer size and fit the physical COMM path more reliably.
  - Keep the stock F' file packet format, stock `FileDownlink`, and the existing COMM downlink write service.
  - Do not add flow-control protocol, a custom GDS plugin, a new COMM service port, or a new wire layout.
- Add a diagnostic TT&C-only mode for the file/downlink probe startup shape.
  - Start the same GDS, gateway, subsystem stack, target OBC, CAN captures, serial preamble, and fprime-cli listeners as the formal file/downlink mode.
  - Do not send `HK_CAPTURE_NOW`, `HK_DOWNLINK_INDEX`, or `HK_DOWNLINK_SLOT` in diagnostic mode.
  - Require three consecutive TT&C cycles before considering the file/downlink startup shape stable enough for formal file assertions.
- Byte comparison governs the file verdict.
  - Snapshot target OBC runtime source files locally immediately before each downlink command.
  - Compare the GDS-received files byte-for-byte against those snapshots.
  - The formal verdict requires `hk-index.csv` plus two occupied slot files.
- Keep evidence boundaries narrow.
  - This proves housekeeping archive file/downlink only.
  - It does not prove arbitrary onboard file downlink, RF, no-preamble behavior, archive wraparound, ScenarioBridge/pass automation, or dual-bus redundancy.

## Risks / Trade-offs

- [Risk] File source data can change while downlink is in progress.
  - Mitigation: snapshot each source file immediately before the matching downlink command and compare against the snapshot.
- [Risk] TT&C visibility could be mistaken for file/downlink proof.
  - Mitigation: the probe must first establish TT&C readiness, then separately require received files and byte matches.
- [Risk] CAN or serial setup failures can look like file/downlink failures.
  - Mitigation: reuse probe-owned CAN bring-up and explicit serial endpoint defaults, preserve CAN health checks in the formal verdict, and run the TT&C-only diagnostic stage before file assertions.
- [Risk] A constrained physical serial carrier can drop stock F' file packets during multi-packet transfers.
  - Mitigation: keep file chunks at `256` bytes, retry bounded `HK_DOWNLINK_*` attempts when a received file does not byte-match, and require final byte-for-byte agreement before accepting the verdict.
- [Risk] This could be over-read as generic file transfer.
  - Mitigation: evidence and registry name only housekeeping archive index plus selected slot files.
