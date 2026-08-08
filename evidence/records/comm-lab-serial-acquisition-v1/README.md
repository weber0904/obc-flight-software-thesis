# comm-lab-serial-acquisition-v1 Evidence

## Scope

This record covers a focused physical serial acquisition proof for the `subsystem.local -> macOS` direction on the current COMM lab wiring.

The formal proof is intentionally below TT&C:

- macOS opens `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` as a passive raw serial receiver.
- `subsystem.local` opens `/dev/serial0` and transmits a bounded ASCII preamble followed by framed marker payloads.
- The receiver scans the stream for `OBCACQ1` records and validates sequence, payload length, and CRC.

This proves a bounded subsystem-origin acquisition strategy for the lab serial link. It does not prove stock F' TT&C, RF, file/downlink, target OBC, or COMM shared CAN FD.

## Guarded TT&C Attempt Context

Before this fallback change, a guarded full TT&C prototype was attempted over:

```text
fprime-cli
  -> fprime-gds
  -> ground_ttc_gateway on macOS
  -> /dev/cu.usbserial-$COMM_SERIAL_DEVICE
  -> physical RS-485 link
  -> subsystem.local:/dev/serial0
  -> native-built comm_csp_node node 4
  -> CSP ZMQHUB
  -> hosted OBC node 1
```

Observed diagnostic result:

- OBC started with `Ground link via COMM CSP node: 4`.
- OBC status showed `groundLink connected=yes`, but with `txErr` and `rxErr`.
- Ground-link events repeatedly showed `GROUND_LINK_DOWN`, `GROUND_LINK_ERROR`, and `GROUND_LINK_UP`.
- OBC readback stayed at the pre-command state: `eps ... pdu=3` and `adcs mode=DETUMBLE`.
- `fprime-cli events` and `fprime-cli channels` did not produce the required event/telemetry proof.

Verdict: `STOPPED`. The attempt did not prove command uplink or downlink over physical lab serial. It is diagnostic context only and is not registered as a TT&C validation path.

## Implemented Probe Entry Point

- `scripts/run_comm_lab_serial_acquisition_probe.sh`

Default command:

```bash
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 \
bash scripts/run_comm_lab_serial_acquisition_probe.sh
```

Default acquisition settings:

| Setting | Value |
|---|---|
| baudrate | `115200` |
| frame count | `8` |
| preamble lines | `20` |
| frame spacing | `500 ms` |
| host timeout | `20 s` |
| frame marker | `OBCACQ1` |
| validation | sequence, payload length, CRC |

## Focused Acquisition Verdict

Verdict: `PASS`.

Final passing run:

| Field | Value |
|---|---|
| command | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 bash scripts/run_comm_lab_serial_acquisition_probe.sh` |
| host serial device | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` |
| subsystem serial device | `/dev/serial0` |
| baudrate | `115200` |
| log directory | `/tmp/obc-comm-lab-acq.Fgx0Po` |

Observed PASS markers:

```text
comm-lab-serial-acquisition-probe: PASS
raw-bytes=1503
decoded-frames=8
expected-frames=8
decoded-frame seq=0 payload=acq-1777524939-27080-0
decoded-frame seq=1 payload=acq-1777524939-27080-1
decoded-frame seq=2 payload=acq-1777524939-27080-2
decoded-frame seq=3 payload=acq-1777524939-27080-3
decoded-frame seq=4 payload=acq-1777524939-27080-4
decoded-frame seq=5 payload=acq-1777524939-27080-5
decoded-frame seq=6 payload=acq-1777524939-27080-6
decoded-frame seq=7 payload=acq-1777524939-27080-7
comm-lab-serial-acquisition-receiver: PASS
comm-lab-serial-acquisition-sender: PASS
```

The raw prefix still contained non-text bytes before clean frame acquisition:

```text
raw-prefix-hex=7e320172b002b26082b24e06368cb28232b0723201824c82028882cc8e9ecc8e928c1c904c3e39c7e34c62c6cc8e800c3e01c7000c3821e07e320172b002b26082b24e06368cb28232b0723201824c82028882cc8e9ecc8eb28c1c904c3e39c7
```

This is why the evidence is scoped to bounded acquisition after preamble/framing, not clean first-byte or stock F' frame transport.

## Diagnostic Runs Before PASS

| Probe shape | Result | Conclusion |
|---|---|---|
| Binary `0x55` preamble with binary magic/length/CRC frames | FAIL; macOS received bytes but decoded `0/8` frames | Continuous binary preamble was not a useful acquisition primitive for this hardware path |
| ASCII frames with default short preamble and `100 ms` spacing | FAIL; macOS received bytes but decoded `0/8` frames | The passive receiver needed longer acquisition/spacing |
| Asserting DTR/RTS in the Python raw receiver/sender | FAIL; no material change | Modem-line assertion alone did not make the short acquisition pass |
| macOS `/dev/tty.usbserial-$COMM_SERIAL_DEVICE` instead of `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` | FAIL; no material change | The endpoint class was not the deciding factor for the short acquisition failure |
| ASCII preamble with `20` preamble lines, `500 ms` frame spacing, and `20 s` timeout | PASS; decoded `8/8` exact frames | Bounded passive acquisition is possible with conservative pacing |

## Not Covered

- gateway-backed TT&C over the physical serial link
- stock F' event, telemetry, or command frames over this link
- simultaneous bidirectional half-duplex behavior
- clean first-byte subsystem-origin traffic
- RF or real radio behavior
- file/downlink behavior
- target OBC migration
- COMM shared CAN FD participation
- `ScenarioBridge`, `ground_pass_open`, or `link_available`

## Verification

Completed on `2026-04-30` after implementation, repeated after rebasing on the merged subsystem UART preflight baseline, and repeated after the review fix that rejects vacuous frame counts:

| Step | Command | Result |
|---|---|---|
| syntax check | `bash -n scripts/run_comm_lab_serial_acquisition_probe.sh` | PASS |
| local gate | `bash scripts/run_verification_ci.sh build-artifacts/comm-lab-serial-acquisition-v1-review-fix` | PASS |
| hosted PTY regression | `GDS_PORT=50330 GDS_TTS_PORT=50331 CSP_HUB_SUB_PORT=56630 CSP_HUB_PUB_PORT=57630 RADIO_PORT=17230 bash scripts/run_comm_csp_ground_gateway_probe.sh` | PASS |
| focused acquisition probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 bash scripts/run_comm_lab_serial_acquisition_probe.sh` | PASS |
| invalid frame count guard | `ACQ_FRAME_COUNT=0`, `ACQ_FRAME_COUNT=-1`, and `ACQ_FRAME_COUNT=abc` with dummy serial paths | PASS; each exits `2` before opening serial endpoints |
| change validation | `openspec validate comm-lab-serial-acquisition-v1` | PASS |
| specs validation | `openspec validate --specs` | PASS |
| archive | `openspec archive comm-lab-serial-acquisition-v1 --yes` | PASS; archived as `openspec/changes/archive/2026-04-29-comm-lab-serial-acquisition-v1/` |

The first hosted PTY regression attempt during initial implementation failed because `0.0.0.0:50161` was already in use by a local process. The rebase closeout used explicit alternate ports and passed.

The local gate summary recorded:

```text
01_generate: PASS
02_build: PASS
03_generate_ut: PASS
04_build_ut: PASS
05_check_all: PASS
06_check_repo_consistency: PASS
07_check_component_test_baseline: PASS
08_check_legacy_zmq_retired: PASS
09_openspec_validate_specs: PASS
```

Archive synced delta specs into `comm-subsystem`, `verification-evidence`, and `verification-path-registry`.
