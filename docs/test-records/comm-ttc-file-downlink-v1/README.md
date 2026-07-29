# comm-ttc-file-downlink-v1 Evidence

## Scope

This record proves bounded file/downlink over the existing COMM TT&C path using only the current housekeeping archive command surface:

- `OBCApp.housekeepingArchive.HK_CAPTURE_NOW`
- `OBCApp.housekeepingArchive.HK_DOWNLINK_INDEX`
- `OBCApp.housekeepingArchive.HK_DOWNLINK_SLOT`

Newly proven physical path:

```text
HK_DOWNLINK_* command
  -> fprime-cli
  -> fprime-gds on macOS
  -> ground_ttc_gateway on macOS
  -> /dev/cu.usbserial-$COMM_SERIAL_DEVICE
  -> physical RS-485/UART lab link
  -> subsystem.local:/dev/serial0
  -> native-built comm_csp_node node 4
  -> CSP ZMQHUB
  -> hosted OBC node 1
  -> HousekeepingArchive -> FileDownlink
  -> COMM downlink write service
  -> physical serial link back through ground_ttc_gateway
  -> fprime-gds file storage
```

The proof keeps the existing COMM contract unchanged:

- COMM node id: `4`
- service `30`: `UPLINK_POLL`
- service `31`: `DOWNLINK_WRITE`
- service `32`: `LINK_STATUS`
- stock F' framing and stock `fprime-gds`
- no new COMM CSP service port
- no new wire layout
- no generic arbitrary-file downlink command

## Not Covered

- arbitrary onboard file path downlink
- RF or real radio behavior
- target OBC migration
- no-preamble first-byte-clean UART behavior
- full archive ring fill, generation wraparound, or retention policy
- ScenarioBridge, `ground_pass_open`, or `link_available`
- COMM shared CAN FD participation

## Implemented Probe Entry Points

- `scripts/run_comm_ttc_file_downlink_probe.sh`
- `scripts/run_comm_csp_file_downlink_probe.sh`
- `scripts/run_comm_lab_serial_file_downlink_probe.sh`

Hosted regression command:

```bash
bash scripts/run_comm_csp_file_downlink_probe.sh
```

Physical formal command:

```bash
PREPARE_SUBSYSTEM_WORKSPACE=0 \
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 \
GDS_PORT=50700 \
GDS_TTS_PORT=50701 \
CSP_HUB_SUB_PORT=56700 \
CSP_HUB_PUB_PORT=57700 \
RADIO_PORT=17700 \
bash scripts/run_comm_lab_serial_file_downlink_probe.sh
```

Both probes:

- start isolated GDS file storage
- run hosted OBC in `GROUND_LINK_MODE=comm-csp`
- use COMM node `4` and existing services `30`, `31`, and `32`
- verify command/event/channel TT&C before file verdict
- issue paced `HK_CAPTURE_NOW` commands until at least two occupied HK slots exist
- downlink `hk-index.csv` and two `hk-slot-*.bin` files
- compare received files byte-for-byte against source snapshots from the OBC runtime tree

## Hosted Regression Verdict

Verdict: `PASS` for hosted PTY COMM TT&C file/downlink regression.

Final passing hosted run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `bash scripts/run_comm_csp_file_downlink_probe.sh` |
| formal verdict | `file-downlink` |
| link mode | `hosted-pty` |
| log directory | `/tmp/obc-comm-csp-file-downlink.Z8Muz2` |
| GDS ports | `50520` / `50521` |
| CSP hub ports | `56520` / `57520` |
| runtime root | `/tmp/comm-csp-file-downlink-runtime` |
| GDS file storage | `/tmp/comm-csp-file-downlink-gds-downlink` |
| HK capture attempts | `12` |
| selected occupied slots | `2` |

Hosted received files:

| File | Source snapshot | Received path | Bytes | SHA-256 |
|---|---|---|---:|---|
| `hk-index.csv` | `/tmp/obc-comm-csp-file-downlink.Z8Muz2/source-snapshots/source-hk-index.csv` | `/tmp/comm-csp-file-downlink-gds-downlink/fprime-downlink/hk-index.csv` | `320` | `af917a7dd265e54916e93b5f0c36240812b9397511bc8907fec8b31923f1f84a` |
| `hk-slot-00-g000000.bin` | `/tmp/obc-comm-csp-file-downlink.Z8Muz2/source-snapshots/source-hk-slot-00-g000000.bin` | `/tmp/comm-csp-file-downlink-gds-downlink/fprime-downlink/hk-slot-00-g000000.bin` | `3872` | `ab9e2e5ec6d55d5a877138bea41858f7115fa4fce28414b079511e3e21659569` |
| `hk-slot-01-g000000.bin` | `/tmp/obc-comm-csp-file-downlink.Z8Muz2/source-snapshots/source-hk-slot-01-g000000.bin` | `/tmp/comm-csp-file-downlink-gds-downlink/fprime-downlink/hk-slot-01-g000000.bin` | `983` | `984c48eac96febf2e108c63ced7cb74825375d3dce194a87f8145cce8f77afb0` |

## Physical Lab Serial Verdict

Verdict: `PASS` for bounded physical lab serial COMM TT&C file/downlink.

Final passing physical run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `PREPARE_SUBSYSTEM_WORKSPACE=0 HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 GDS_PORT=50700 GDS_TTS_PORT=50701 CSP_HUB_SUB_PORT=56700 CSP_HUB_PUB_PORT=57700 RADIO_PORT=17700 bash scripts/run_comm_lab_serial_file_downlink_probe.sh` |
| formal verdict | `file-downlink` |
| link mode | `physical-serial` |
| log directory | `/tmp/obc-comm-lab-serial-file-downlink.LXWiqL` |
| host serial device | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` |
| subsystem serial device | `/dev/serial0` |
| baudrate | `115200` |
| COMM node | `4` |
| local CSP hub host | `127.0.0.1` |
| remote CSP hub host | `<private-lab-host>` |
| CSP hub ports | `56700` / `57700` |
| GDS ports | `50700` / `50701` |
| radio port | `17700` |
| gateway serial TX preamble | `20` lines, `500 ms` delay |
| startup delay before commands | `12 s` |
| bounded TT&C command attempts | `6` |
| file downlink command attempts | `3` |
| file START timeout | `20 s` |
| file byte-match timeout | `60 s` |
| HK capture attempts | `24` |
| selected occupied slots | `2` |

Physical received files:

| File | Source snapshot | Received path | Bytes | SHA-256 |
|---|---|---|---:|---|
| `hk-index.csv` | `/tmp/obc-comm-lab-serial-file-downlink.LXWiqL/source-snapshots/source-hk-index.csv` | `/tmp/comm-lab-serial-file-downlink-gds-downlink/fprime-downlink/hk-index.csv` | `320` | `5f0e9f2b8206c3027651c2b230d074424dfe1a76e795871eb8b9e484d622ac26` |
| `hk-slot-00-g000000.bin` | `/tmp/obc-comm-lab-serial-file-downlink.LXWiqL/source-snapshots/source-hk-slot-00-g000000.bin` | `/tmp/comm-lab-serial-file-downlink-gds-downlink/fprime-downlink/hk-slot-00-g000000.bin` | `3872` | `7eb8b7090b229a226ff6cf0101b4567d707eb816d38d932a2ada7fa997bd5aff` |
| `hk-slot-01-g000000.bin` | `/tmp/obc-comm-lab-serial-file-downlink.LXWiqL/source-snapshots/source-hk-slot-01-g000000.bin` | `/tmp/comm-lab-serial-file-downlink-gds-downlink/fprime-downlink/hk-slot-01-g000000.bin` | `983` | `e53843a473e0d12e5c7b6a40d8f2290489d22875225ba27aea065d543b203d50` |

Observed physical PASS markers:

```text
comm-ttc-file-downlink-probe: PASS
formal-verdict=file-downlink
link-mode=physical-serial
stage0-ttc-prerequisite=PASS
downlinked-index=PASS
downlinked-slots=2
comm-lab-serial-file-downlink-probe: PASS
formal-verdict=file-downlink
```

## TT&C Prerequisite

The physical probe reused the already registered command/event/channel TT&C path and required it before file assertions. It sent bounded prerequisite commands:

```text
OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true
OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING
```

The final physical OBC readback confirmed both command effects:

```text
Ground link via COMM CSP node: 4
groundLink mode=comm-csp commNode=4
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.00,-0.00,0.00) pointingErr=0.00
groundLink connected=yes tx=12366 rx=784 txErr=3 rxErr=14
```

The final physical run also observed command events and `GROUND_LINK_TX_BYTES` telemetry through `fprime-cli`.

## Diagnostic Notes

- Physical serial remains bounded by acquisition preamble and retries. This evidence does not prove no-preamble first-byte-clean behavior.
- The probe records source snapshots because `HousekeepingArchive` can continue updating `index.csv` and the active slot while file transfers are in progress.
- A diagnostic physical attempt that selected two closed full slots exceeded the intended multi-slot scope and exposed packet gaps on the current lab serial path. The final formal probe returns to the planned requirement: at least two occupied HK slots, not full ring fill or generation wraparound.
- A diagnostic run on GDS ports `50544` / `50545` stopped because those ports were already held by a local IDE language service. The formal PASS used explicit free ports `50700` / `50701`.

## Supporting And Adjacent Baselines

Reused/supporting baselines:

- Hosted gateway-backed omitted-RF COMM TT&C:
  [docs/test-records/comm-csp-node-and-ground-gateway-v1/README.md](../comm-csp-node-and-ground-gateway-v1/README.md)
- Physical lab serial command/event/channel TT&C:
  [docs/test-records/ttc-over-comm-lab-serial-downlink-v1/README.md](../ttc-over-comm-lab-serial-downlink-v1/README.md)
- Physical lab serial uplink ingress:
  [docs/test-records/ttc-over-comm-lab-serial-ingress-v1/README.md](../ttc-over-comm-lab-serial-ingress-v1/README.md)
- macOS-initiated physical UART request/reply preflight:
  [docs/test-records/subsystem-comm-uart-link-preflight-v1/README.md](../subsystem-comm-uart-link-preflight-v1/README.md)

This evidence newly proves housekeeping archive file/downlink over the existing physical lab serial COMM TT&C path. It keeps arbitrary file downlink, RF, target OBC migration, ScenarioBridge, no-preamble first-byte-clean behavior, archive wraparound, and COMM shared CAN FD as future work.

## Verification

Closeout checks:

| Step | Command | Result |
|---|---|---|
| syntax check | `bash -n scripts/run_comm_ttc_file_downlink_probe.sh` | PASS |
| syntax check | `bash -n scripts/run_comm_csp_file_downlink_probe.sh` | PASS |
| syntax check | `bash -n scripts/run_comm_lab_serial_file_downlink_probe.sh` | PASS |
| fresh local gate | `bash scripts/run_verification_ci.sh build-artifacts/comm-ttc-file-downlink-v1-closeout` | PASS |
| hosted PTY file/downlink regression | `bash scripts/run_comm_csp_file_downlink_probe.sh` | PASS |
| physical lab serial file/downlink probe | `PREPARE_SUBSYSTEM_WORKSPACE=0 HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 GDS_PORT=50700 GDS_TTS_PORT=50701 CSP_HUB_SUB_PORT=56700 CSP_HUB_PUB_PORT=57700 RADIO_PORT=17700 bash scripts/run_comm_lab_serial_file_downlink_probe.sh` | PASS |

