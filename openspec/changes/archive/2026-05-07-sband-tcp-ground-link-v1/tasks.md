## 1. Formalize the change boundary

- [x] 1.1 Create proposal, design, and delta spec artifacts for the S-band TCP through-COMM path.
- [x] 1.2 Validate the active OpenSpec change before implementation closeout.

## 2. Implement S-band TCP endpoint support

- [x] 2.1 Add TCP listen support to shared COMM node app/server configuration while preserving serial-device behavior.
- [x] 2.2 Add TCP southbound client support to `ground_ttc_gateway` while preserving serial-device behavior.
- [x] 2.3 Ensure `sband_comm_csp_node` can run as node `5` with a TCP listen endpoint and reviewable startup logs.
- [x] 2.4 Keep generic node `4`, UHF node `6`, services `30-32`, and COMM packet layouts unchanged.

## 3. Add S-band hosted probe coverage

- [x] 3.1 Add `scripts/run_sband_tcp_ground_link_probe.sh` with isolated GDS, CSP hub, S-band TCP, runtime, file-storage, and log paths.
- [x] 3.2 Verify bounded command/event/channel TT&C through `fprime-cli -> GDS -> gateway -> S-band TCP -> sband_comm_csp_node(node 5) -> OBC`.
- [x] 3.3 Verify bounded housekeeping archive file/downlink over the same S-band TCP path with received files byte-matched against source snapshots.
- [x] 3.4 Verify the probe output and logs distinguish GDS TCP from S-band simulated TCP and do not use direct OBC-to-GDS as the verdict.

## 4. Update docs and evidence

- [x] 4.1 Update simulator and script README surfaces for the new TCP endpoint and S-band probe.
- [x] 4.2 Add `docs/test-records/sband-tcp-ground-link-v1/README.md` with TT&C and file/downlink proof boundaries.
- [x] 4.3 Add hosted S-band TCP TT&C and hosted S-band TCP file/downlink entries to `docs/verification-path-registry.md`.

## 5. Verify, archive, and prepare review boundary

- [x] 5.1 Run focused local tests for `comm_sim_model_unit_test` and `comm_groundlink_unit_test`.
- [x] 5.2 Run `bash scripts/run_sband_tcp_ground_link_probe.sh`.
- [x] 5.3 Run `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-sband-tcp-ground-link-v1`.
- [x] 5.4 Rerun `bash scripts/run_sband_tcp_ground_link_probe.sh` after the fresh build.
- [x] 5.5 Run `openspec validate sband-tcp-ground-link-v1` and `openspec validate --specs`.
- [x] 5.6 Archive the change, update the reconciliation matrix, and rerun repo consistency checks.
