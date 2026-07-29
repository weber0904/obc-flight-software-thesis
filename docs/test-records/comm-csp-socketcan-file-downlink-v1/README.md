# comm-csp-socketcan-file-downlink-v1 Evidence

## Scope

This record proves housekeeping archive file/downlink over the existing SocketCAN-backed COMM TT&C path using only the current housekeeping archive command surface:

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
  -> comm_csp_node node 4 on subsystem.local:can1
  -> shared CAN FD-capable bus
  -> obc.local:can0
  -> target OBC node 1
  -> HousekeepingArchive -> stock F' FileDownlink
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

The formal run used project-local `FW_FILE_BUFFER_MAX_SIZE = 256` so stock F' file data packets stayed bounded for the constrained physical COMM path.

## Not Covered

- arbitrary onboard file path downlink
- RF or real radio behavior
- no-preamble first-byte-clean UART behavior
- full archive ring fill, generation wraparound, or retention policy
- ScenarioBridge, `ground_pass_open`, or `link_available`
- dual-bus redundancy
- independent COMM hardware beyond the `subsystem.local:can1` controller
- end-to-end missing-packet retransmission, NACK/ARQ recovery, or reliable file transfer under packet loss

## Implemented Probe Entry Points

- `scripts/run_shared_canfd_eps_adcs_health_probe.sh`
- `scripts/run_comm_csp_socketcan_ttc_probe.sh`
- `scripts/run_comm_csp_socketcan_file_downlink_probe.sh`
- `scripts/run_comm_csp_file_downlink_probe.sh`

Stage 0 health command:

```bash
OBC_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_RESERVED_CAN_DEVICE=can1 \
bash scripts/run_shared_canfd_eps_adcs_health_probe.sh
```

TT&C diagnostic command:

```bash
COMM_SOCKETCAN_FILE_PROBE_MODE=ttc-prereq \
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 \
OBC_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 \
bash scripts/run_comm_csp_socketcan_file_downlink_probe.sh
```

Formal physical command:

```bash
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 \
OBC_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 \
bash scripts/run_comm_csp_socketcan_file_downlink_probe.sh
```

The physical probes bring required CAN interfaces `UP` before judging connectivity, using:

```text
bitrate 500000 dbitrate 2000000 fd on
```

## Stage 0 Health Verdict

Verdict: `PASS` for the existing EPS/ADCS SocketCAN path before formal file/downlink.

Final passing Stage 0 run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_RESERVED_CAN_DEVICE=can1 bash scripts/run_shared_canfd_eps_adcs_health_probe.sh` |
| formal verdict | `eps-adcs-health` |
| log directory | `/tmp/obc-shared-canfd-health.YBgNnV` |
| OBC CAN | `obc.local:can0` |
| subsystem EPS/ADCS CAN | `subsystem.local:can0` |
| subsystem reserved/observed CAN | `subsystem.local:can1` |

Observed Stage 0 checks:

```text
csp ping response=0 success=yes
csp ping response=0 success=yes
eps soc=76.00 vbat=8.06 tempBat=25.00 pdu=3
adcs mode=DETUMBLE omega=(0.00,-0.00,0.00) pointingErr=0.00
```

## SocketCAN TT&C Control Verdict

Verdict: `PASS` for the existing physical SocketCAN command/event/channel TT&C control probe after the file-probe hardening.

Final passing control run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_ttc_probe.sh` |
| formal verdict | `bounded-ttc` |
| link mode | `physical-socketcan` |
| log directory | `/tmp/obc-comm-csp-socketcan-ttc.ZqDQdZ` |
| COMM node | `4` |
| GDS ports | `51900` / `51901` |
| gateway serial TX preamble | `20` lines, `500 ms` delay |

Observed control PASS markers:

```text
comm-csp-socketcan-ttc-probe: PASS
formal-verdict=bounded-ttc
comm-ping=PASS
eps-adcs-ping=PASS
reset-command-readback=PASS
stage1-command-readback=PASS
stage2-event-channel-downlink=PASS
```

## Diagnostic TT&C Verdict

Verdict: `PASS` for TT&C under the file/downlink probe startup load, without sending HK/file commands.

Final passing diagnostic run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `COMM_SOCKETCAN_FILE_PROBE_MODE=ttc-prereq HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_file_downlink_probe.sh` |
| diagnostic verdict | `ttc-prereq` |
| link mode | `physical-socketcan` |
| log directory | `/tmp/obc-comm-csp-socketcan-file-downlink.EIz5cb` |
| required cycles | `3` |
| passed cycles | `3` |
| CAN capture timeout | `25 s` |
| prepare remote runtime | `1` |

Observed diagnostic PASS markers:

```text
comm-csp-socketcan-file-downlink-probe: PASS
probe-mode=ttc-prereq
ttc-prereq-cycles-required=3
ttc-prereq-cycles-passed=3
stage2-event-channel-downlink=PASS
```

## Hosted Regression Verdict

Verdict: `PASS` for the hosted PTY COMM file/downlink regression after bounding the F' file buffer.

Final passing hosted run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `bash scripts/run_comm_csp_file_downlink_probe.sh` |
| formal verdict | `file-downlink` |
| link mode | `hosted-pty` |
| log directory | `/tmp/obc-comm-csp-file-downlink.JGh8N5` |
| GDS ports | `50520` / `50521` |
| CSP hub ports | `56520` / `57520` |
| runtime root | `/tmp/comm-csp-file-downlink-runtime` |
| GDS file storage | `/tmp/comm-csp-file-downlink-gds-downlink` |
| HK capture attempts | `12` |
| selected occupied slots | `2` |

Hosted received files:

| File | Source snapshot | Received path | Bytes | SHA-256 |
|---|---|---|---:|---|
| `hk-index.csv` | `/tmp/obc-comm-csp-file-downlink.JGh8N5/source-snapshots/source-hk-index.csv` | `/tmp/comm-csp-file-downlink-gds-downlink/fprime-downlink/hk-index.csv` | `319` | `debe6136dea8a164dd6adb4f9d2c00b21fc1b194445ea405c35fb6dc802f9df3` |
| `hk-slot-00-g000000.bin` | `/tmp/obc-comm-csp-file-downlink.JGh8N5/source-snapshots/source-hk-slot-00-g000000.bin` | `/tmp/comm-csp-file-downlink-gds-downlink/fprime-downlink/hk-slot-00-g000000.bin` | `3872` | `16f362d84bcb3f287d250033e1a57c44fb3f559b4d770c4813b3c1afeb0cd34b` |
| `hk-slot-01-g000000.bin` | `/tmp/obc-comm-csp-file-downlink.JGh8N5/source-snapshots/source-hk-slot-01-g000000.bin` | `/tmp/comm-csp-file-downlink-gds-downlink/fprime-downlink/hk-slot-01-g000000.bin` | `983` | `3e36cb3e338e56b505b46a5b012e6ab09937fb3053265fa5dbaedcba6cd44358` |

## Physical SocketCAN File/Downlink Verdict

Verdict: `PASS` for bounded housekeeping archive file/downlink over the physical SocketCAN-backed COMM path.

Final passing physical run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_file_downlink_probe.sh` |
| formal verdict | `file-downlink` |
| link mode | `physical-socketcan` |
| log directory | `/tmp/obc-comm-csp-socketcan-file-downlink.kykgvh` |
| host serial device | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` |
| subsystem serial device | `/dev/serial0` |
| baudrate | `115200` |
| OBC CAN | `obc.local:can0` |
| subsystem EPS/ADCS CAN | `subsystem.local:can0` |
| subsystem COMM CAN | `subsystem.local:can1` |
| COMM node | `4` |
| GDS ports | `51900` / `51901` |
| GDS file storage | `/tmp/comm-csp-socketcan-file-downlink-gds-downlink` |
| runtime root | `$OBC_HOME/lab/fprime/v0/runtime/comm-csp-socketcan-file-downlink` |
| gateway serial TX preamble | `20` lines, `500 ms` delay |
| CAN timing | `500000` nominal bitrate, `2000000` data bitrate, CAN FD on |
| file data buffer | `FW_FILE_BUFFER_MAX_SIZE = 256` |
| file downlink command attempts | `3` |
| HK capture attempts | `16` |
| selected occupied slots | `2` |

Physical received files:

| File | Source snapshot | Received path | Bytes | SHA-256 |
|---|---|---|---:|---|
| `hk-index.csv` | `/tmp/obc-comm-csp-socketcan-file-downlink.kykgvh/source-snapshots/source-hk-index.csv` | `/tmp/comm-csp-socketcan-file-downlink-gds-downlink/fprime-downlink/hk-index.csv` | `320` | `d0f2f4d4a1aa63516851d9b31f670db23bec666de70a45f548972ba66d1b9d30` |
| `hk-slot-01-g000000.bin` | `/tmp/obc-comm-csp-socketcan-file-downlink.kykgvh/source-snapshots/source-hk-slot-01-g000000.bin` | `/tmp/comm-csp-socketcan-file-downlink-gds-downlink/fprime-downlink/hk-slot-01-g000000.bin` | `1304` | `9b8defa929bab3ed68a6e8cd977942a8244dca235be86590a5db933832cf7f59` |
| `hk-slot-00-g000000.bin` | `/tmp/obc-comm-csp-socketcan-file-downlink.kykgvh/source-snapshots/source-hk-slot-00-g000000.bin` | `/tmp/comm-csp-socketcan-file-downlink-gds-downlink/fprime-downlink/hk-slot-00-g000000.bin` | `3872` | `298eac6de65c5960cd203299ccc7b126944f90977c68cb81e0da1875dcc653ad` |

Observed physical PASS markers:

```text
comm-csp-socketcan-file-downlink-probe: PASS
probe-mode=file-downlink
formal-verdict=file-downlink
stage0-ttc-prerequisite=PASS
downlinked-index=PASS
downlinked-slots=2
slot-match=PASS slot=1 generation=0
slot-match=PASS slot=0 generation=0
```

CAN captures were non-empty on all active interfaces:

| Interface | Capture | Lines |
|---|---|---:|
| `obc.local:can0` | `/tmp/obc-comm-csp-socketcan-file-downlink.kykgvh/obc.candump.log` | `11376` |
| `subsystem.local:can0` | `/tmp/obc-comm-csp-socketcan-file-downlink.kykgvh/subsystem-eps-adcs.candump.log` | `10933` |
| `subsystem.local:can1` | `/tmp/obc-comm-csp-socketcan-file-downlink.kykgvh/subsystem-comm.candump.log` | `11549` |

Post-run CAN health:

| Interface | Parent device | State | Bus-off counter | RX packets | TX packets |
|---|---|---|---:|---:|---:|
| `obc.local:can0` | `spi0.0` | `ERROR-ACTIVE` | `0` | `1643568` | `591166` |
| `subsystem.local:can0` | `spi0.0` | `ERROR-ACTIVE` | `0` | `1058996` | `72398` |
| `subsystem.local:can1` | `spi1.0` | `ERROR-ACTIVE` | `0` | `1080275` | `617692` |

## Diagnostic Notes

- The earlier file/downlink failures were not classified as a physical CAN outage: Stage 0 health, the existing SocketCAN TT&C control probe, and the file-probe `ttc-prereq` mode all passed after the probe waited for a connected COMM ground-link state.
- The file probe now starts from a fresh remote runtime by default. A stale target runtime could leave old archive state and background traffic that made early file verdicts ambiguous.
- GDS port probing now uses an actual TCP/UDP bind check. This caught port ownership that `lsof` did not report consistently.
- The probe records source snapshots immediately before each downlink command because `HousekeepingArchive` can continue updating `index.csv` and the active slot while transfers are in progress.
- A diagnostic formal run with 499-byte F' file data packets exposed repeated GDS sequence gaps on the full 3872-byte slot. The formal PASS used `FW_FILE_BUFFER_MAX_SIZE = 256`, producing 243-byte data packets, plus bounded `HK_DOWNLINK_*` retries gated by final byte matches.
- The final physical run needed bounded file/downlink retries for the active slot before the byte-match passed. The full closed slot then downlinked in sequence and byte-matched.
- The current path does not implement end-to-end missing-packet retransmission. Stock F' file packet sequence ids let GDS detect gaps, and the probe may retry a whole `HK_DOWNLINK_*` command within its attempt limit, but this evidence does not prove packet-loss recovery for an individual missing file data packet.
- The current formal proof still uses the physical serial gateway acquisition preamble of 20 lines and 500 ms delay. It does not claim no-preamble first-byte-clean behavior.

## Supporting And Adjacent Baselines

Reused/supporting baselines:

- Physical COMM SocketCAN command/event/channel TT&C:
  [docs/test-records/comm-csp-socketcan-participation-v1/README.md](../comm-csp-socketcan-participation-v1/README.md)
- Physical lab serial COMM file/downlink:
  [docs/test-records/comm-ttc-file-downlink-v1/README.md](../comm-ttc-file-downlink-v1/README.md)
- Shared physical EPS/ADCS SocketCAN foundation:
  [docs/test-records/shared-canfd-csp-bus-foundation-v1/README.md](../shared-canfd-csp-bus-foundation-v1/README.md)
- Physical lab serial command/event/channel TT&C:
  [docs/test-records/ttc-over-comm-lab-serial-downlink-v1/README.md](../ttc-over-comm-lab-serial-downlink-v1/README.md)

This evidence newly proves housekeeping archive file/downlink over the existing physical SocketCAN-backed COMM TT&C path. It keeps arbitrary file downlink, RF, no-preamble behavior, archive wraparound, ScenarioBridge/pass automation, dual-bus redundancy, independent COMM hardware, and packet-loss-tolerant reliable file transfer as future work.

## Verification

Closeout checks:

| Step | Command | Result |
|---|---|---|
| syntax check | `bash -n scripts/run_comm_csp_socketcan_file_downlink_probe.sh` | PASS |
| syntax check | `bash -n scripts/run_comm_csp_socketcan_ttc_probe.sh` | PASS |
| OpenSpec change validation | `openspec validate comm-csp-socketcan-file-downlink-v1` | PASS |
| OpenSpec specs validation | `openspec validate --specs` | PASS |
| repo consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| diff whitespace check | `git diff --check` | PASS |
| Stage 0 EPS/ADCS health | `OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_RESERVED_CAN_DEVICE=can1 bash scripts/run_shared_canfd_eps_adcs_health_probe.sh` | PASS |
| physical SocketCAN TT&C control probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_ttc_probe.sh` | PASS |
| TT&C under file-probe startup load | `COMM_SOCKETCAN_FILE_PROBE_MODE=ttc-prereq HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_file_downlink_probe.sh` | PASS |
| hosted COMM file/downlink regression | `bash scripts/run_comm_csp_file_downlink_probe.sh` | PASS |
| formal physical SocketCAN file/downlink probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_file_downlink_probe.sh` | PASS |
| full local gate | `bash scripts/run_verification_ci.sh build-artifacts/comm-csp-socketcan-file-downlink-v1-closeout` | PASS |
