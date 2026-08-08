# target-obc-comm-csp-lab-operational-baseline-v1 Evidence

## Scope

This record proves the service-managed lab target operational baseline for the previously proven COMM SocketCAN target path.

Newly proven operational path:

```text
macOS fprime-cli / fprime-gds
  -> ground_ttc_gateway on macOS
  -> /dev/cu.usbserial-$COMM_SERIAL_DEVICE
  -> physical RS-485/UART lab link
  -> subsystem.local:/dev/serial0
  -> subsystem.local COMM node 4 on can1
  -> shared CAN FD-capable SocketCAN bus
  -> obc.local can0
  -> installed OBC release under obc-comm-csp-stack.service
  -> EPS node 2 and ADCS node 3 on subsystem.local can0
  -> COMM downlink back through the same service-managed path
  -> fprime-gds file storage
```

The baseline is operational for the lab target path, but it is not a final flight deployment baseline. The subsystem workspace services and CAN oneshot services are transitional lab mechanisms.

## Not Covered

- final flight deployment baseline
- RF behavior or real radio behavior
- no-preamble first-byte-clean behavior on the current physical serial link
- reliable retransmission, NACK/ARQ, or recovery of an individual lost file packet
- arbitrary onboard file path downlink
- ScenarioBridge, `ground_pass_open`, `link_available`, or pass automation
- final OS-level CAN provisioning
- dedicated non-login service users such as `obc-runtime` or `subsystem-runtime`

## Release Package Boundary

OBC installable bundle:

| Field | Value |
|---|---|
| package release id | `v0.1.0-110-gae2f47b` |
| package tarball | `build-artifacts/packages/rpi/v0.1.0-110-gae2f47b/obc-rpi-v0.1.0-110-gae2f47b.tar.gz` |
| manifest | `build-artifacts/packages/rpi/v0.1.0-110-gae2f47b/manifest.json` |
| package created at UTC | `2026-05-02T17:39:22+00:00` |
| OBC binary SHA-256 | `3b1684869066e8dc511dbd51961abeca7fbbcb03e7ca22744b77d0c3759f017b` |
| OBC comm-csp launch profile SHA-256 | `97591f4ec737e1c8ee805b694ee24bf379c5da73ac0235bcddd50cd4c8179e44` |
| OBC comm-csp service template SHA-256 | `f5d97de56cd30e526c7a4610236bb1f530cda0ca44e9545e01c64a0315c52f36` |

The OBC tarball contains the OBC runtime, dictionary, metadata, `launch/run_obc_comm_csp_stack.sh`, and `systemd/obc-comm-csp-stack.service.template`.

The subsystem service templates, CAN oneshot templates, ground launcher, and operator runbook are repo-governed lab deployment artifacts outside the OBC tarball boundary.

## Implemented Entry Points

- `scripts/package_rpi_bundle.sh`
- `scripts/install_rpi_bundle.sh`
- `scripts/install_rpi_comm_csp_autostart.sh`
- `scripts/rpi_comm_csp_status.sh`
- `scripts/install_lab_can_services.sh`
- `scripts/lab_can_status.sh`
- `scripts/install_subsystem_comm_csp_services.sh`
- `scripts/subsystem_comm_csp_status.sh`
- `scripts/run_target_comm_csp_ground_stack.sh`
- `scripts/run_target_comm_csp_lab_operational_probe.sh`
- `docs/operator/target-lab.md`

## Service-Managed Verdict

Verdict: `PASS` for service-managed OBC, subsystem, CAN, and ground surfaces.

Final passing hardware run:

| Field | Value |
|---|---|
| date | `2026-05-03` |
| command | `bash scripts/run_target_comm_csp_lab_operational_probe.sh` |
| log directory | `/tmp/target-obc-comm-csp-lab-operational.k8b54o` |
| branch commit under test before final docs/archive | `804be3269841dcd4143a392acab74deac3cda5d0` |
| OBC target | `operator@obc.local` |
| subsystem target | `operator@subsystem.local` |
| runtime root | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc` |
| COMM node | `4` |
| reboot autostart proof | `PASS` |

OBC service state after reboot:

| Service | State | Runtime user | Notes |
|---|---|---|---|
| `obc-comm-csp-stack.service` | enabled, active running since `2026-05-03 02:17:21 CST` | `operator` | started `$OBC_HOME/obc-deploy/current/bin/OBC` from the installed release |
| `obc-installed-stack.service` | disabled, inactive | n/a | stopped to avoid racing the lab COMM CSP service |

Subsystem service state:

| Service | State | Runtime user | Node / device |
|---|---|---|---|
| `subsystem-comm-csp-stack.target` | enabled, active since `2026-05-03 02:17:03 CST` | n/a | aggregate target |
| `subsystem-eps-csp.service` | enabled, active running | `operator` | EPS node `2` on `can0` |
| `subsystem-adcs-csp.service` | enabled, active running | `operator` | ADCS node `3` on `can0` |
| `subsystem-comm-csp.service` | enabled, active running | `operator` | COMM node `4` on `can1`, serial ingress `/dev/serial0 @ 115200` |

CAN oneshot service state:

| Service | Device | State |
|---|---|---|
| `obc-lab-can.service` | `obc.local:can0` | enabled, active exited |
| `subsystem-eps-adcs-lab-can.service` | `subsystem.local:can0` | enabled, active exited |
| `subsystem-comm-lab-can.service` | `subsystem.local:can1` | enabled, active exited |

The CAN oneshots used the lab default timing:

```text
bitrate 500000 dbitrate 2000000 fd on
```

Post-run CAN interface state:

| Interface | Parent device | State | Bus-off counter | RX packets | TX packets |
|---|---|---|---:|---:|---:|
| `obc.local:can0` | `spi0.0` | `ERROR-ACTIVE` | `0` | `6166` | `2080` |
| `subsystem.local:can0` | `spi0.0` | `ERROR-ACTIVE` | `0` | `1175917` | `65460` |
| `subsystem.local:can1` | `spi1.0` | `ERROR-ACTIVE` | `0` | `1175989` | `677193` |

CAN captures were non-empty on all active interfaces:

| Interface | Capture | Lines |
|---|---|---:|
| `obc.local:can0` | `/tmp/target-obc-comm-csp-lab-operational.k8b54o/obc.candump.log` | `13952` |
| `subsystem.local:can0` | `/tmp/target-obc-comm-csp-lab-operational.k8b54o/subsystem-eps-adcs.candump.log` | `13900` |
| `subsystem.local:can1` | `/tmp/target-obc-comm-csp-lab-operational.k8b54o/subsystem-comm.candump.log` | `13902` |

## Ground And E2E Verdict

Verdict: `PASS` for the macOS ground launcher plus end-to-end command, event, channel, GPS, and file/downlink validation.

Ground launcher settings:

| Field | Value |
|---|---|
| launcher | `scripts/run_target_comm_csp_ground_stack.sh` |
| GDS bind | `0.0.0.0:51900` |
| GDS TTS port | `51901` |
| GDS file storage | `/tmp/target-obc-comm-csp-lab-operational-gds-downlink` |
| gateway GDS target | `127.0.0.1:51900` |
| host serial endpoint | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE @ 115200` |
| serial TX preamble | `20` lines, `500 ms` delay |

Observed PASS markers:

```text
target-obc-comm-csp-lab-operational-probe: PASS
obc-autostart=PASS
can-state=PASS
service-runtime-user=PASS
eps-command-readback=PASS
adcs-command-readback=PASS
stage2-event-channel-downlink=PASS
gps-source=LIVE_UART accepted-before=4 accepted-after=6
downlinked-index=PASS
downlinked-slots=2
```

The probe reset controlled subsystem state, then sent the bounded target commands through the service-managed GDS/COMM/SocketCAN path:

```text
OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true
OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING
```

Ground-side event and channel evidence included command completion events, `EPS_PDU_STATUS = 7`, `ADCS_MODE = POINTING`, and nonzero `GROUND_LINK_TX_BYTES`.

Live GPS UART was enabled through the installed OBC service defaults. The accepted-sentence count advanced from `4` to `6`; a valid fix was not required or claimed.

## File Matches

The final run byte-matched the housekeeping archive index and two occupied archive slot files against target OBC runtime source snapshots.

| File | Source | Source snapshot | Received path | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `hk-index.csv` | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc/hk/index.csv` | `/tmp/target-obc-comm-csp-lab-operational.k8b54o/source-snapshots/source-hk-index.csv` | `/tmp/target-obc-comm-csp-lab-operational-gds-downlink/fprime-downlink/hk-index.csv` | `587` | `08721c93a8aaf0353a5a88de0c42ff131cc47213bd7b635277b46e0eb95da51f` |
| `hk-slot-03-g000000.bin` | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc/hk/hk-03.bin` | `/tmp/target-obc-comm-csp-lab-operational.k8b54o/source-snapshots/source-hk-slot-03-g000000.bin` | `/tmp/target-obc-comm-csp-lab-operational-gds-downlink/fprime-downlink/hk-slot-03-g000000.bin` | `1946` | `23163d0bf06ebbcfa30050155313e1940642cc0eeec10b89b6977bc572f7aab8` |
| `hk-slot-01-g000000.bin` | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc/hk/hk-01.bin` | `/tmp/target-obc-comm-csp-lab-operational.k8b54o/source-snapshots/source-hk-slot-01-g000000.bin` | `/tmp/target-obc-comm-csp-lab-operational-gds-downlink/fprime-downlink/hk-slot-01-g000000.bin` | `2267` | `fc8d466ffa931b05d7c3e8ba7bde4a089110b12fe24a7a2b0bf2446f4062da60` |

## Diagnostic Notes

- The final run required bounded whole-command file/downlink retries before the index and final slot byte-matched. This remains evidence of bounded operational retry behavior in the probe, not reliable packet-level retransmission.
- The path keeps the existing COMM contract: node `4`, services `30 UPLINK_POLL`, `31 DOWNLINK_WRITE`, and `32 LINK_STATUS`, stock F' framing, and stock `fprime-gds`.
- The OBC runtime and subsystem runtime processes ran as `operator`, not root. This is accepted only for the current workspace-owned lab service stage.
- Root was used only for CAN interface provisioning through systemd oneshot services.
- The OBC service defaults use `GROUND_LINK_MODE=comm-csp`, `COMM_CSP_NODE=4`, `CSP_TRANSPORT=socketcan`, `CSP_CAN_DEVICE=can0`, `CSP_CAN_PROMISC=0`, `OBC_GPS_SOURCE_MODE=live-uart`, `OBC_GPS_SERIAL_DEVICE=/dev/serial0`, and `OBC_GPS_BAUDRATE=9600`.

## Supporting And Adjacent Baselines

Reused/supporting baselines:

- Physical COMM SocketCAN command/event/channel TT&C:
  [evidence/records/comm-csp-socketcan-participation-v1/README.md](../comm-csp-socketcan-participation-v1/README.md)
- Physical COMM SocketCAN housekeeping archive file/downlink:
  [evidence/records/comm-csp-socketcan-file-downlink-v1/README.md](../comm-csp-socketcan-file-downlink-v1/README.md)
- OBC live GPS UART:
  [evidence/records/gps-live-uart-source-v1/README.md](../gps-live-uart-source-v1/README.md)
  and [evidence/records/gps-live-uart-hardening-v1/README.md](../gps-live-uart-hardening-v1/README.md)
- Shared physical EPS/ADCS SocketCAN foundation:
  [evidence/records/shared-canfd-csp-bus-foundation-v1/README.md](../shared-canfd-csp-bus-foundation-v1/README.md)

This evidence newly proves the service-managed lab target operational path. It does not introduce a new COMM wire contract.

## Verification

Closeout checks:

| Step | Command | Result |
|---|---|---|
| package OBC bundle | `bash scripts/package_rpi_bundle.sh` | PASS; produced `v0.1.0-110-gae2f47b` |
| install OBC bundle | `bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/v0.1.0-110-gae2f47b/obc-rpi-v0.1.0-110-gae2f47b.tar.gz` | PASS |
| full local gate | `bash scripts/run_verification_ci.sh build-artifacts/target-obc-comm-csp-lab-operational-baseline-v1-closeout` | PASS |
| post-gate hardware operational probe | `bash scripts/run_target_comm_csp_lab_operational_probe.sh` | PASS |

The full local gate passed:

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
