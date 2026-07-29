## 1. Formalize the change boundary

- [x] 1.1 Create the `ccsds-ground-link-spike` OpenSpec proposal, design, and delta specs.
- [x] 1.2 Validate the active OpenSpec change before implementation work proceeds.

## 2. Add spike-only CCSDS topology support

- [x] 2.1 Add a spike-only CCSDS OBC topology/executable or build target that imports `ComCcsds.Subtopology`.
- [x] 2.2 Wire `GroundLinkDriver` to `ComCcsds.comStub` and command, event, telemetry, file uplink, and file downlink paths through `ComCcsds`.
- [x] 2.3 Preserve the default `OBC` `ComFprime` topology and do not change S-band node `5`, UHF node `6`, or COMM services `30-32`.
- [x] 2.4 Keep the CCSDS spike runtime on `GROUND_LINK_MODE=comm-csp` and `COMM_CSP_NODE=5`; keep direct GDS TCP outside the verdict.

## 3. Add CCSDS hosted probe coverage

- [x] 3.1 Add `scripts/run_ccsds_ground_link_spike_probe.sh` with isolated runtime, GDS, gateway, CSP hub, S-band node `5`, OBC, file-storage, and logs.
- [x] 3.2 Start `fprime-gds` with `--framing-selection space-packet-space-data-link`, `--scid 0x44`, `--vcid 1`, and `--frame-size 1024`.
- [x] 3.3 Verify bounded `EPS_SET_PDU`, `ADCS_SET_MODE`, command events, `GROUND_LINK_TX_BYTES`, `HK_DOWNLINK_INDEX`, and at least two `HK_DOWNLINK_SLOT` byte matches.
- [x] 3.4 Emit explicit probe markers for verdict, COMM node, framing, SCID, VCID, and frame size.

## 4. Record evidence and registry outcome

- [x] 4.1 Add `evidence/records/ccsds-ground-link-spike-v1/README.md` with path, framing, APID/sequence observations, file byte matches, exclusions, and recommendation.
- [x] 4.2 Register the CCSDS hosted path in `evidence/verification-path-registry.md` only if the hosted proof passes.
- [x] 4.3 If the proof fails or is inconclusive, record blockers and do not add a reusable registry path.

## 5. Verify and prepare review boundary

- [x] 5.1 Run `openspec validate ccsds-ground-link-spike` and `openspec validate --specs`.
- [x] 5.2 Run the CCSDS spike build/generate path.
- [x] 5.3 Run `comm_groundlink_unit_test` and `comm_sim_model_unit_test`.
- [x] 5.4 Run `bash scripts/run_ccsds_ground_link_spike_probe.sh`.
- [x] 5.5 Rerun `bash scripts/run_sband_tcp_ground_link_probe.sh` to prove the stock `ComFprime` S-band baseline remains PASS.
- [x] 5.6 Run `bash scripts/run_uhf_uart_backup_link_probe.sh` if shared COMM code changes.
- [x] 5.7 Run `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-ccsds-ground-link-spike`.
