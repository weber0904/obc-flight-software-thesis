## 1. Formalize the change boundary

- [x] 1.1 Create proposal, design, and delta spec artifacts for the dual-link COMM simulator foundation.
- [x] 1.2 Validate the active OpenSpec change before implementation closeout.

## 2. Implement explicit COMM simulator identities

- [x] 2.1 Add shared COMM node app support so executable wrappers can set default link identity, node id, and CSP interface name.
- [x] 2.2 Preserve `comm_csp_node` as generic compatibility node `4`.
- [x] 2.3 Add `sband_comm_csp_node` with default node `5` and `uhf_comm_csp_node` with default node `6`.
- [x] 2.4 Keep services `30-32` and request/reply packet layouts unchanged.

## 3. Add hosted foundation probe coverage

- [x] 3.1 Add a focused live COMM service probe helper that validates `UPLINK_POLL`, `DOWNLINK_WRITE`, and `LINK_STATUS` for a selected node.
- [x] 3.2 Add `scripts/run_comm_dual_link_sim_foundation_probe.sh` with isolated ports, runtime roots, PTYs, and per-link logs.
- [x] 3.3 Verify the probe checks OBC CSP ping reachability for nodes `4`, `5`, and `6` and service behavior for each node.

## 4. Update docs and evidence

- [x] 4.1 Update simulator and script README surfaces for the new executable and probe names.
- [x] 4.2 Add `evidence/records/comm-dual-link-sim-foundation-v1/README.md` with narrow proof boundaries.
- [x] 4.3 Add the hosted dual-link COMM simulator foundation path to `evidence/verification-path-registry.md`.

## 5. Verify, archive, and prepare review boundary

- [x] 5.1 Run focused local tests for `comm_sim_model_unit_test` and `comm_groundlink_unit_test`.
- [x] 5.2 Run `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-comm-dual-link-sim-foundation`.
- [x] 5.3 Rerun `bash scripts/run_comm_dual_link_sim_foundation_probe.sh` after the fresh build.
- [x] 5.4 Run `openspec validate comm-dual-link-sim-foundation` and `openspec validate --specs`.
- [x] 5.5 Archive the change, update the reconciliation matrix, and rerun repo consistency checks.
