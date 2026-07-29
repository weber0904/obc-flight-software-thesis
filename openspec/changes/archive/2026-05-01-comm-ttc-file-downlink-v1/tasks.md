## 1. OpenSpec Setup

- [x] 1.1 Create the governed OpenSpec change on branch `feature/comm-ttc-file-downlink-v1`.
- [x] 1.2 Add proposal, design, delta specs, and implementation tasks for COMM TT&C file/downlink proof.

## 2. File Downlink Probes

- [x] 2.1 Add a hosted PTY COMM file/downlink probe that starts isolated GDS, gateway, COMM node, hosted OBC, and GDS file storage.
- [x] 2.2 Add a physical lab serial strict probe that defaults to `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` and `subsystem.local:/dev/serial0` with environment overrides.
- [x] 2.3 Implement paced `HK_CAPTURE_NOW` orchestration until at least two housekeeping archive slots are occupied.
- [x] 2.4 Downlink `HK_DOWNLINK_INDEX` and two `HK_DOWNLINK_SLOT` files through the COMM path and compare received files byte-for-byte with runtime source files.
- [x] 2.5 Keep probe output explicit about endpoints, ports, preamble settings, slot selections, file paths, byte comparisons, and formal verdict.

## 3. Evidence And Documentation

- [x] 3.1 Add `evidence/records/comm-ttc-file-downlink-v1/README.md` after the hosted and physical probes pass.
- [x] 3.2 Register physical lab serial COMM file/downlink in `evidence/verification-path-registry.md` after the formal physical probe passes.
- [x] 3.3 Update `docs/planning/comm-roadmap.md` to move file/downlink out of Next and leave COMM SocketCAN participation as the next roadmap item.

## 4. Verification And Closeout

- [x] 4.1 Run `bash -n` on new/changed probe scripts and `git diff --check`.
- [x] 4.2 Run the fresh local gate: `bash scripts/run_verification_ci.sh build-artifacts/comm-ttc-file-downlink-v1-closeout`.
- [x] 4.3 Run the hosted PTY COMM file/downlink regression after the fresh build.
- [x] 4.4 Run the physical lab serial COMM file/downlink probe after the fresh build with `PREPARE_SUBSYSTEM_WORKSPACE=0`.
- [x] 4.5 Run `openspec validate comm-ttc-file-downlink-v1` and `openspec validate --specs`.
- [x] 4.6 Archive the OpenSpec change, update reconciliation surfaces, and prepare the local-ready git boundary without pushing.
