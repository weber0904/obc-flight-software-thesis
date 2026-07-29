## 1. OpenSpec Setup

- [x] 1.1 Create the governed change on branch `feature/comm-csp-socketcan-file-downlink-v1`.
- [x] 1.2 Add proposal, design, delta specs, and implementation tasks.
- [x] 1.3 Commit OpenSpec proposal/design/spec deltas before probe implementation.

## 2. Formal SocketCAN File/Downlink Probe

- [x] 2.1 Add `scripts/run_comm_csp_socketcan_file_downlink_probe.sh`.
- [x] 2.2 Reuse the proven SocketCAN TT&C startup path with CAN bring-up, GDS, gateway, subsystem EPS/ADCS + COMM stack, and target OBC in `comm-csp` mode.
- [x] 2.3 Require TT&C prerequisite checks before file assertions.
- [x] 2.4 Generate at least two occupied HK archive slots with paced `HK_CAPTURE_NOW`.
- [x] 2.5 Downlink `hk-index.csv` and two selected `hk-slot-*.bin` files, then byte-match each received file against a source snapshot from the target OBC runtime tree.
- [x] 2.6 Preserve active CAN capture and CAN health checks from the SocketCAN TT&C probe.
- [x] 2.7 Add a `ttc-prereq` diagnostic mode that uses the same startup shape as formal file/downlink mode but sends no HK/file commands.
- [x] 2.8 Require three consecutive diagnostic TT&C cycles before running the formal file/downlink proof.
- [x] 2.9 Harden the physical SocketCAN TT&C probes so command attempts wait for connected COMM ground-link state and avoid stale-log ping matches.
- [x] 2.10 Bound stock F' file data packets to the constrained physical COMM path and keep file verdict retries byte-match gated.

## 3. Evidence And Closeout

- [x] 3.1 Run syntax checks, Stage 0 CAN health, the existing physical SocketCAN TT&C control probe, and the new `ttc-prereq` diagnostic mode.
- [x] 3.2 Run the full local gate, hosted COMM file/downlink regression, and the formal physical SocketCAN file/downlink probe only after diagnostic TT&C passes.
- [x] 3.3 Add `evidence/records/comm-csp-socketcan-file-downlink-v1/README.md` after the formal probe passes.
- [x] 3.4 Register the new verification path and update roadmap/reconciliation surfaces after archive.
- [x] 3.5 Run OpenSpec validations, repo consistency check, and `git diff --check`.
- [x] 3.6 Archive the OpenSpec change and prepare a local-ready branch without pushing.
