# sband-tcp-ground-link-v1 Evidence

## Scope

This record proves the first hosted S-band TCP ground-link path through the explicit S-band COMM simulator identity:

```text
fprime-cli
  -> stock fprime-gds TCP endpoint
  -> ground_ttc_gateway
  -> simulated S-band TCP segment
  -> sband_comm_csp_node node 5
  -> governed internal CSP substrate
  -> hosted OBC node 1
  -> COMM downlink back through the same S-band TCP path
  -> fprime-gds file storage
```

The proof uses `sband_comm_csp_node` as CSP node `5`; it is not direct `GDS -> TCP -> OBC` evidence. Generic `comm_csp_node` node `4` compatibility and UHF node `6` foundation behavior remain preserved but are not newly proven by this record.

Newly proven scope:

- bounded command/event/channel TT&C through `fprime-cli -> GDS -> ground_ttc_gateway -> S-band TCP -> sband_comm_csp_node(node 5) -> OBC`
- bounded housekeeping archive file/downlink over the same S-band TCP through-COMM path
- received `hk-index.csv` and two selected `hk-slot-*.bin` files byte-matched against OBC runtime source snapshots
- distinct GDS TCP and simulated S-band TCP endpoints in launcher/probe output and logs

## Not Covered

- UHF UART/RS485/USB/macOS backup behavior
- CCSDS behavior
- RF behavior or real radio behavior
- reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
- target hardware behavior
- Raspberry Pi deployment
- pass scheduling or contact automation
- arbitrary onboard file path downlink
- full S-band operational deployment beyond this hosted TCP simulation

## Implemented Entry Points

- `sband_comm_csp_node --tcp-listen-host <host> --tcp-listen-port <port> --node-id 5`
- `ground_ttc_gateway --rf-tcp-host <host> --rf-tcp-port <port> --link-identity sband`
- `scripts/run_sband_tcp_ground_link_probe.sh`
- `scripts/run_comm_ttc_file_downlink_probe.sh` with `COMM_TTC_FILE_LINK_MODE=hosted-sband-tcp`

## Hosted S-band TCP Verdict

Verdict: `PASS` for bounded S-band TCP command/event/channel TT&C and housekeeping archive file/downlink.

Post-gate passing hosted run:

| Field | Value |
|---|---|
| date | `2026-05-07` |
| command | `bash scripts/run_sband_tcp_ground_link_probe.sh` |
| formal verdict | `file-downlink` |
| link mode | `hosted-sband-tcp` |
| log directory | `/tmp/obc-sband-tcp-ground-link.Vgy0tQ` |
| S-band TCP endpoint | `127.0.0.1:18520` |
| GDS ports | `50520` / `50521` |
| CSP hub ports | `56520` / `57520` |
| runtime root | `/tmp/sband-tcp-ground-link-runtime` |
| GDS file storage | `/tmp/sband-tcp-ground-link-gds-downlink` |
| COMM executable | `sband_comm_csp_node` |
| COMM node | `5` |
| COMM interface | `SBANDCSP` |
| HK capture attempts | `10` |
| selected occupied slots | `2` |

Observed startup identity lines:

```text
COMM node startup: executable=sband_comm_csp_node link=sband node=5 endpoint=tcp-listen tcp=127.0.0.1:18520 interface=SBANDCSP
ground_ttc_gateway startup: link=sband gds=127.0.0.1:50520 southbound=tcp-client rf-tcp=127.0.0.1:18520
```

Observed probe PASS markers:

```text
comm-ttc-file-downlink-probe: PASS
formal-verdict=file-downlink
link-mode=hosted-sband-tcp
stage0-ttc-prerequisite=PASS
sband-tcp-endpoint=127.0.0.1:18520
comm-node=5
downlinked-index=PASS
downlinked-slots=2
sband-tcp-ground-link-probe: PASS
formal-verdict=file-downlink
```

## File Matches

The hosted run byte-matched the housekeeping archive index and two occupied archive slot files against hosted OBC runtime source snapshots.

| File | Source snapshot | Received path | Bytes | SHA-256 |
|---|---|---|---:|---|
| `hk-index.csv` | `/tmp/obc-sband-tcp-ground-link.Vgy0tQ/source-snapshots/source-hk-index.csv` | `/tmp/sband-tcp-ground-link-gds-downlink/fprime-downlink/hk-index.csv` | `320` | `4b38ed2b2a8d1e9dae17b623a8076427e661de96e9954e87fac65e412aeffc9b` |
| `hk-slot-00-g000000.bin` | `/tmp/obc-sband-tcp-ground-link.Vgy0tQ/source-snapshots/source-hk-slot-00-g000000.bin` | `/tmp/sband-tcp-ground-link-gds-downlink/fprime-downlink/hk-slot-00-g000000.bin` | `4030` | `10dd2cb8dd37560cd96fb32b55f107aeb0381bb68da23f6e123ab1d44791990f` |
| `hk-slot-01-g000000.bin` | `/tmp/obc-sband-tcp-ground-link.Vgy0tQ/source-snapshots/source-hk-slot-01-g000000.bin` | `/tmp/sband-tcp-ground-link-gds-downlink/fprime-downlink/hk-slot-01-g000000.bin` | `822` | `bd570946c2d06f2414a3731adb0a4534b81c6138245341d7b2c02a0f6be6c889` |

## TT&C Prerequisite

The probe required bounded command/event/channel TT&C before file assertions. It sent controlled commands through the S-band TCP through-COMM path:

```text
OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true
OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING
```

Observed OBC readback confirmed the command effects and the selected COMM node:

```text
Ground link via COMM CSP node: 5
groundLink mode=comm-csp commNode=5
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.04,-0.03,0.02) pointingErr=0.00
groundLink connected=yes tx=3155 rx=291 txErr=0 rxErr=1
```

The run also observed command dispatch/completion events and nonzero `GROUND_LINK_TX_BYTES` telemetry through `fprime-cli`.

## Verification

Closeout checks:

| Step | Command | Result |
|---|---|---|
| focused groundlink unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_groundlink_unit_test` | PASS |
| focused model unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_sim_model_unit_test` | PASS |
| initial hosted S-band TCP probe | `bash scripts/run_sband_tcp_ground_link_probe.sh` | PASS |
| full local gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-sband-tcp-ground-link-v1` | PASS after rerun; initial attempt hit transient ADCS CSP default-port bind residue, and `run_adcs_csp_integration.sh` passed independently before the clean full-gate rerun |
| post-gate hosted S-band TCP probe | `bash scripts/run_sband_tcp_ground_link_probe.sh` | PASS |
| post-review TCP peer-close regression | `./build-fprime-automatic-native-ut/bin/Darwin/comm_groundlink_unit_test` | PASS; includes TCP peer reset write returning `false` instead of terminating on `SIGPIPE` |
| post-review hosted S-band TCP probe | `bash scripts/run_sband_tcp_ground_link_probe.sh` | PASS |
| post-review full local gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-sband-tcp-ground-link-v1` | PASS |
