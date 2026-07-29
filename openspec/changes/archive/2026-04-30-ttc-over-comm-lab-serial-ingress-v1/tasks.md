## 1. Setup And Baseline

- [x] 1.1 Validate the OpenSpec change before implementation.
- [x] 1.2 Create the staged probe skeleton without claiming hardware success.
- [x] 1.3 Commit OpenSpec artifacts and probe skeleton as the first local rollback point.

## 2. Stage 1 Uplink Ingress

- [x] 2.1 Implement Stage 0 prerequisite checks for macOS-initiated UART request/reply.
- [x] 2.2 Implement Stage 1 orchestration for local GDS, gateway, CSP hub, hosted OBC, and remote native `comm_csp_node`.
- [x] 2.3 Assert bounded OBC readback for `EPS_SET_PDU` and `ADCS_SET_MODE`.
- [x] 2.4 Record Stage 1 logs and PASS/STOP verdict in evidence.

## 3. Stage 2 Downlink Attempt

- [x] 3.1 Add `fprime-cli events` and `fprime-cli channels` listeners after Stage 1 succeeds.
- [x] 3.2 Upgrade evidence to full bounded TT&C only if events and telemetry are visible.
- [x] 3.3 If Stage 2 fails, keep the formal verdict at uplink ingress and record downlink diagnostics.

## 4. Closeout

- [x] 4.1 Run the fresh local verification gate.
- [x] 4.2 Run hosted PTY gateway regression after the fresh build.
- [x] 4.3 Run physical support/precondition probes and the staged lab serial ingress probe.
- [x] 4.4 Validate OpenSpec change and specs.
- [x] 4.5 Sync/archive the change, update the verification path registry and reconciliation matrix, and prepare the final PR boundary.
