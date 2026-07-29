## 1. OpenSpec Setup

- [x] 1.1 Create the governed change on branch `feature/comm-csp-socketcan-participation-v1`.
- [x] 1.2 Add proposal, design, delta specs, and implementation tasks.

## 2. CAN Bring-Up And Stack Support

- [x] 2.1 Add shared script helper logic that brings Linux CAN interfaces `UP` with explicit SocketCAN timing before hardware probes.
- [x] 2.2 Add a corrected Stage 0 EPS/ADCS SocketCAN health smoke that does not require `can1` isolation.
- [x] 2.3 Extend the OBC CAN launcher to support `GROUND_LINK_MODE=comm-csp` and `COMM_CSP_NODE=4`.
- [x] 2.4 Add a subsystem stack entrypoint that runs EPS/ADCS on `can0` and COMM node `4` on `can1`.

## 3. Formal COMM SocketCAN TT&C Probe

- [x] 3.1 Add `scripts/run_comm_csp_socketcan_ttc_probe.sh`.
- [x] 3.2 Have the probe bring up `obc.local:can0`, `subsystem.local:can0`, and `subsystem.local:can1`.
- [x] 3.3 Verify COMM node `4` reachability and bounded command/event/channel TT&C through GDS and the gateway.
- [x] 3.4 Record CAN captures and pre/post interface health for all active CAN interfaces.

## 4. Evidence And Closeout

- [x] 4.1 Add `docs/test-records/comm-csp-socketcan-participation-v1/README.md` after the physical probe passes.
- [x] 4.2 Register the new path in `docs/verification-path-registry.md`.
- [x] 4.3 Update roadmap and reconciliation surfaces after archive.
- [x] 4.4 Run syntax checks, full local gate, physical probe, OpenSpec validations, repo consistency check, and `git diff --check`.
- [x] 4.5 Archive the OpenSpec change and prepare a local-ready branch without pushing.
