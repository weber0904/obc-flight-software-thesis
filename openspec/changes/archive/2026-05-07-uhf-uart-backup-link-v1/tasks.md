## 1. Formalize and review the change boundary

- [x] 1.1 Create proposal, design, and delta spec artifacts for UHF hosted UART backup command ingress and UHF node-6 beacon side-channel capture.
- [x] 1.2 Run `openspec validate uhf-uart-backup-link-v1`.
- [x] 1.3 Stop for developer scope review before implementation.

## 2. Implement UHF beacon side-channel support

- [x] 2.1 Add bounded node-6 BeaconV1 push support from hosted OBC to `uhf_comm_csp_node` without changing COMM services `30-32`.
- [x] 2.2 Add hosted UHF beacon serial output/capture support with explicit executable, link identity, node id, endpoint, and baudrate logs.
- [x] 2.3 Keep existing OBC transparent-passive beacon behavior intact unless documentation must clarify the distinction.

## 3. Add UHF hosted backup ingress coverage

- [x] 3.1 Add `scripts/run_uhf_uart_backup_link_probe.sh` with isolated GDS ports, CSP hub ports, runtime root, PTY serial pairs, and logs.
- [x] 3.2 Verify bounded command/event/channel ingress through `fprime-cli -> fprime-gds -> ground_ttc_gateway(link=uhf serial) -> PTY -> uhf_comm_csp_node(node 6) -> OBC`.
- [x] 3.3 Verify OBC runs with `GROUND_LINK_MODE=comm-csp`, `COMM_CSP_NODE=6`, and direct OBC-to-GDS disabled or outside the verdict boundary.
- [x] 3.4 Verify node `4` generic compatibility and node `5` S-band TCP behavior remain unchanged.

## 4. Add UHF beacon capture coverage

- [x] 4.1 Extend the UHF hosted probe to capture at least one BeaconV1 frame from the UHF node-6 beacon side channel.
- [x] 4.2 Decode the captured BeaconV1 frame and verify fixed wire size, schema version, CRC, and representative populated state fields.
- [x] 4.3 Keep beacon evidence separate from command ingress evidence and avoid claiming file/downlink, reliable transfer, or full downlink authority.

## 5. Update docs and evidence

- [x] 5.1 Update simulator and script README surfaces for UHF hosted UART backup ingress and UHF beacon side-channel capture.
- [x] 5.2 Add `evidence/records/uhf-uart-backup-link-v1/README.md` with command ingress and beacon proof boundaries.
- [x] 5.3 Add hosted UHF serial backup TT&C ingress and UHF node-6 beacon path entries to `evidence/verification-path-registry.md`.

## 6. Verify, archive, and prepare review boundary

- [x] 6.1 Run focused local tests for COMM gateway/model and any BeaconV1 or beacon-publisher surfaces touched.
- [x] 6.2 Run `bash scripts/run_uhf_uart_backup_link_probe.sh`.
- [x] 6.3 Run `bash scripts/run_comm_dual_link_sim_foundation_probe.sh`.
- [x] 6.4 Run `bash scripts/run_sband_tcp_ground_link_probe.sh`.
- [x] 6.5 Run `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-uhf-uart-backup-link-v1`.
- [x] 6.6 Rerun `bash scripts/run_uhf_uart_backup_link_probe.sh` after the fresh build.
- [x] 6.7 Run `openspec validate uhf-uart-backup-link-v1` and `openspec validate --specs`.
- [x] 6.8 Archive the change, update the reconciliation matrix, and rerun repo consistency checks.
