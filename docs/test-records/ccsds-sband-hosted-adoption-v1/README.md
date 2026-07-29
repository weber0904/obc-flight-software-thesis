# CCSDS S-band Hosted Adoption v1

This record governs the formal adoption of the hosted S-band CCSDS path as the default hosted `OBC` ground path.

## Decision

- Default hosted binary: `OBC`
- Default operator namespace: `OBCApp.*`
- Default hosted ground path: `fprime-gds(CCSDS) -> ground_ttc_gateway(raw relay) -> S-band TCP -> sband_comm_csp_node(node 5) -> OBC`
- GDS framing: `space-packet-space-data-link`
- SCID: `0x44`
- VCID: `1`
- TM frame size: `1024`
- Legacy hosted `ComFprime` binary: `OBC_ComFprimeLegacy`
- Legacy hosted `ComFprime` namespace: `OBCAppComFprimeLegacy.*`

`ccsds-ground-link-spike-v1` remains historical spike evidence. This record is the governed default-path adoption evidence.

## Scope

Proves:

- default hosted `OBC` uses the CCSDS topology while retaining `OBCApp.*`
- S-band node `5` remains the COMM node for the hosted S-band TCP segment
- `ground_ttc_gateway` remains a transparent raw-byte relay between GDS and S-band TCP
- bounded commands, events, telemetry, and housekeeping archive file/downlink traverse the CCSDS S-band path
- captured TC/TM traffic decodes with expected SCID, VCID, TM frame size, and APID flows for command `0`, telemetry `1`, event/log `2`, and file `3`

Does not prove:

- UHF CCSDS
- RF behavior
- reliable transfer, CFDP, ARQ, NACK, or retry
- target/Pi deployment
- command authority, session, authentication, failover, or pass policy
- arbitrary onboard file downlink
- repository-level reliable transfer from CCSDS sequence counts

## Verification Commands

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util generate -f
PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build
python3 scripts/test_decode_ccsds_capture.py
bash scripts/run_ccsds_sband_hosted_adoption_probe.sh
bash scripts/run_sband_tcp_ground_link_probe.sh
bash scripts/run_uhf_uart_backup_link_probe.sh
```

## Results

Fresh local verification:

- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-ccsds-sband-hosted-adoption-v1`: PASS
  - `01_generate`: PASS
  - `02_build`: PASS
  - `03_generate_ut`: PASS
  - `04_build_ut`: PASS
  - `05_check_all`: PASS
  - `06_check_repo_consistency`: PASS
  - `07_check_component_test_baseline`: PASS
  - `08_check_legacy_zmq_retired`: PASS
  - `09_openspec_validate_specs`: PASS

Focused tests:

- `./build-fprime-automatic-native-ut/bin/Darwin/hosted_runtime_unit_test`: PASS
- `./build-fprime-automatic-native-ut/bin/Darwin/comm_groundlink_unit_test`: PASS
- `python3 scripts/test_decode_ccsds_capture.py`: PASS

Focused probes:

- `bash scripts/run_ccsds_sband_hosted_adoption_probe.sh`: PASS
  - formal verdict: `ccsds-hosted-sband-adoption`
  - log directory: `/tmp/obc-ccsds-sband-adoption.V54mB6`
  - default binary: `OBC`
  - command prefix: `OBCApp`
  - COMM node: `5`
  - framing: `space-packet-space-data-link`
  - SCID/VCID/frame size: `0x44` / `1` / `1024`
  - decoded APIDs: command `0`, telemetry `1`, event/log `2`, file `3`
  - capture summary: `/tmp/obc-ccsds-sband-adoption.V54mB6/ccsds-capture-summary.json`
- `bash scripts/run_sband_tcp_ground_link_probe.sh`: PASS
  - formal verdict: `file-downlink`
  - log directory: `/tmp/obc-sband-tcp-ground-link.wLH1aW`
  - pinned binary/prefix/framing: `OBC_ComFprimeLegacy` / `OBCAppComFprimeLegacy` / `fprime`
- `bash scripts/run_uhf_uart_backup_link_probe.sh`: PASS
  - formal verdict: `bounded-ttc-and-beacon`
  - log directory: `/tmp/obc-uhf-uart-backup.m2xwaF`
  - pinned binary/prefix/link: `OBC_ComFprimeLegacy` / `OBCAppComFprimeLegacy` / `hosted-uhf-pty`

The CCSDS capture records observed frame and packet sequence counts for decode evidence only. This record does not claim reliable transfer, retry, ARQ, NACK, CFDP, or repository-level delivery guarantees from sequence counts.
