## 1. OpenSpec Setup

- [x] 1.1 Create the governed OpenSpec change on branch `feature/ttc-over-comm-lab-serial-downlink-v1`.
- [x] 1.2 Add proposal, design, delta specs, and implementation tasks for the bounded physical lab serial downlink proof.

## 2. Downlink Probe

- [x] 2.1 Add a focused `scripts/run_comm_lab_serial_downlink_probe.sh` entrypoint.
- [x] 2.2 Reuse the current physical lab serial topology and Stage 1 EPS/ADCS command readback checks.
- [x] 2.3 Make Stage 2 event/channel observations mandatory for the downlink probe PASS verdict.
- [x] 2.4 Keep probe output and logs explicit about endpoints, ports, preamble settings, command attempts, and formal verdict.

## 3. Evidence And Documentation

- [x] 3.1 Add `evidence/records/ttc-over-comm-lab-serial-downlink-v1/README.md`.
- [x] 3.2 Register the bounded physical lab serial TT&C path in `evidence/verification-path-registry.md` after the focused probe passes.
- [x] 3.3 Update `docs/planning/comm-roadmap.md` to move this item out of Next and leave `comm-ttc-file-downlink-v1` as the next COMM roadmap item.

## 4. Verification And Closeout

- [x] 4.1 Run `bash -n scripts/run_comm_lab_serial_downlink_probe.sh` and `git diff --check`.
- [x] 4.2 Run any affected unit tests if implementation touches COMM helper or `GroundLinkDriver` code. No C++ helper/component code changed; full gate still ran the baseline test set.
- [x] 4.3 Run the fresh local gate: `bash scripts/run_verification_ci.sh build-artifacts/ttc-over-comm-lab-serial-downlink-v1-closeout`.
- [x] 4.4 Run hosted PTY gateway regression after the fresh build.
- [x] 4.5 Run the physical lab serial downlink probe after the fresh build.
- [x] 4.6 Run `openspec validate ttc-over-comm-lab-serial-downlink-v1` and `openspec validate --specs`.
- [x] 4.7 Archive the OpenSpec change and prepare the local-ready git boundary without pushing.
