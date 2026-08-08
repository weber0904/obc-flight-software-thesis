# comm-csp-socketcan-participation-v1 Evidence

## Scope

This record proves bounded command/event/channel TT&C through COMM node `4` after COMM joins the shared SocketCAN carrier through `subsystem.local:can1`.

Newly proven physical path:

```text
fprime-cli command
  -> fprime-gds on macOS
  -> ground_ttc_gateway on macOS
  -> /dev/cu.usbserial-$COMM_SERIAL_DEVICE
  -> physical RS-485/UART lab link
  -> subsystem.local:/dev/serial0
  -> comm_csp_node node 4 on subsystem.local:can1
  -> shared CAN FD-capable bus
  -> obc.local:can0
  -> target OBC node 1
  -> COMM downlink back through the same COMM/gateway/GDS path
  -> fprime-cli events/channels
```

The proof keeps the existing COMM service contract unchanged:

- COMM node id: `4`
- service `30`: `UPLINK_POLL`
- service `31`: `DOWNLINK_WRITE`
- service `32`: `LINK_STATUS`
- stock F' framing and stock `fprime-gds`
- no new COMM CSP service port
- no new wire layout
- no generic arbitrary-file or file/downlink command

## Not Covered

- file/downlink over the SocketCAN-backed COMM path
- arbitrary onboard file path downlink
- RF or real radio behavior
- no-preamble first-byte-clean serial behavior
- dual-bus redundancy
- independent COMM hardware beyond the `subsystem.local:can1` controller
- ScenarioBridge, `ground_pass_open`, or `link_available`

## Implemented Probe Entry Points

- `scripts/run_shared_canfd_eps_adcs_health_probe.sh`
- `scripts/run_rpi_can_csp_stack.sh`
- `scripts/run_subsystem_sim_comm_can_stack.sh`
- `scripts/run_subsystem_sim_remote_comm_can_stack.sh`
- `scripts/run_comm_csp_socketcan_ttc_probe.sh`

Corrected Stage 0 health command:

```bash
OBC_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_RESERVED_CAN_DEVICE=can1 \
bash scripts/run_shared_canfd_eps_adcs_health_probe.sh
```

Formal physical command:

```bash
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 \
OBC_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 \
SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 \
bash scripts/run_comm_csp_socketcan_ttc_probe.sh
```

Both hardware probes bring required CAN interfaces `UP` before judging connectivity, using:

```text
bitrate 500000 dbitrate 2000000 fd on
```

Setup failures such as a missing CAN interface or failed `sudo -n ip link set ... up` are not treated as wiring verdicts.

## Stage 0 Health Verdict

Verdict: `PASS` for the existing EPS/ADCS SocketCAN path after `subsystem.local:can1` was physically attached to the same bus but not used by Stage 0.

Final passing Stage 0 run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_RESERVED_CAN_DEVICE=can1 bash scripts/run_shared_canfd_eps_adcs_health_probe.sh` |
| formal verdict | `eps-adcs-health` |
| log directory | `/tmp/obc-shared-canfd-health.b3WuAy` |
| OBC CAN | `obc.local:can0` |
| subsystem EPS/ADCS CAN | `subsystem.local:can0` |
| subsystem reserved/observed CAN | `subsystem.local:can1` |

Observed Stage 0 checks:

```text
csp ping node 2 success 1
csp ping node 3 success 1
eps soc=76.00 vbat=8.06 tempBat=25.00 pdu=3
adcs mode=DETUMBLE omega=(0.00,-0.00,0.00) pointingErr=0.00
```

Stage 0 CAN health:

- `obc.local:can0`: `ERROR-ACTIVE`, no bus-off, post-run RX `2808` packets / TX `944` packets.
- `subsystem.local:can0`: `ERROR-ACTIVE`, no bus-off, post-run RX `2808` packets / TX `1864` packets.
- `subsystem.local:can1`: present as `spi1.0`, left `DOWN` by the Stage 0 EPS/ADCS health smoke because Stage 0 does not use the old reserved-isolation verdict after rewiring.

## Physical COMM SocketCAN TT&C Verdict

Verdict: `PASS` for bounded command/event/channel TT&C through COMM node `4` on `subsystem.local:can1` and target OBC on `obc.local:can0`.

Final passing formal run:

| Field | Value |
|---|---|
| date | `2026-05-02` |
| command | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_ttc_probe.sh` |
| formal verdict | `bounded-ttc` |
| link mode | `physical-socketcan` |
| log directory | `/tmp/obc-comm-csp-socketcan-ttc.79ecVJ` |
| host serial device | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` |
| subsystem serial device | `/dev/serial0` |
| baudrate | `115200` |
| OBC CAN | `obc.local:can0` |
| subsystem EPS/ADCS CAN | `subsystem.local:can0` |
| subsystem COMM CAN | `subsystem.local:can1` |
| COMM node | `4` |
| GDS ports | `51900` / `51901` |
| gateway serial TX preamble | `20` lines, `500 ms` delay |
| CAN timing | `500000` nominal bitrate, `2000000` data bitrate, CAN FD on |

Observed formal PASS markers:

```text
comm-csp-socketcan-ttc-probe: PASS
formal-verdict=bounded-ttc
link-mode=physical-socketcan
comm-ping=PASS
eps-adcs-ping=PASS
reset-command-readback=PASS
stage1-command-readback=PASS
stage2-event-channel-downlink=PASS
```

The probe first reset the controlled subsystem state through the same GDS/COMM/SocketCAN path:

```text
eps soc=76.00 vbat=8.06 tempBat=25.00 pdu=3
adcs mode=DETUMBLE omega=(0.00,-0.00,0.00) pointingErr=0.00
```

It then sent the formal target commands through `fprime-cli`:

```text
OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true
OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING
```

Final OBC readback:

```text
groundLink mode=comm-csp commNode=4
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.00,-0.00,0.00) pointingErr=0.00
groundLink connected=yes tx=5361 rx=1042 txErr=15 rxErr=5
```

Ground-side event and telemetry observations through `fprime-cli`:

```text
CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10033001 dispatched to port 11
CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10033001 completed
CdhCore.cmdDisp.OpCodeDispatched : Opcode 0x10034000 dispatched to port 7
CdhCore.cmdDisp.OpCodeCompleted : Opcode 0x10034000 completed
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 205
```

CAN captures were non-empty on all active interfaces:

| Interface | Capture | Lines |
|---|---|---:|
| `obc.local:can0` | `/tmp/obc-comm-csp-socketcan-ttc.79ecVJ/obc.candump.log` | `11244` |
| `subsystem.local:can0` | `/tmp/obc-comm-csp-socketcan-ttc.79ecVJ/subsystem-eps-adcs.candump.log` | `10921` |
| `subsystem.local:can1` | `/tmp/obc-comm-csp-socketcan-ttc.79ecVJ/subsystem-comm.candump.log` | `11448` |

Post-run CAN health:

| Interface | Parent device | State | Bus-off counter | RX packets | TX packets |
|---|---|---|---:|---:|---:|
| `obc.local:can0` | `spi0.0` | `ERROR-ACTIVE` | `0` | `159305` | `56367` |
| `subsystem.local:can0` | `spi0.0` | `ERROR-ACTIVE` | `0` | `153157` | `13736` |
| `subsystem.local:can1` | `spi1.0` | `ERROR-ACTIVE` | `0` | `155374` | `89026` |

## Diagnostic Notes

- The earlier Stage 0 false failure was caused by judging CAN while interfaces were still `DOWN`. The corrected probe brings the required interfaces `UP` before testing and classifies bring-up failure separately from wiring failure.
- A first formal attempt failed because the `obc.local` target workspace still had a stale OBC binary that did not support `--ground-link`; rebuilding `obc.local` fixed that setup issue.
- A later diagnostic attempt showed that simulator state can carry over between lab runs. The final probe therefore uses a reset-to-opposite-state step before the formal target commands.
- The final probe requires each `candump` subprocess to complete successfully and each capture log to contain real CAN frame lines before accepting CAN evidence.
- The current formal proof still uses the physical serial gateway acquisition preamble of 20 lines and 500 ms delay. It does not claim no-preamble first-byte-clean behavior.
- `subsystem.local:can1` is connected in parallel to the shared CAN_H/CAN_L bus for this proof. This is not dual-bus redundancy.

## Supporting And Adjacent Baselines

Reused/supporting baselines:

- Shared physical EPS/ADCS SocketCAN foundation:
  [evidence/records/shared-canfd-csp-bus-foundation-v1/README.md](../shared-canfd-csp-bus-foundation-v1/README.md)
- Physical lab serial bounded COMM TT&C:
  [evidence/records/ttc-over-comm-lab-serial-downlink-v1/README.md](../ttc-over-comm-lab-serial-downlink-v1/README.md)
- Physical lab serial COMM file/downlink, adjacent but not reused as proof for file/downlink here:
  [evidence/records/comm-ttc-file-downlink-v1/README.md](../comm-ttc-file-downlink-v1/README.md)
- Hosted gateway-backed omitted-RF COMM TT&C:
  [evidence/records/comm-csp-node-and-ground-gateway-v1/README.md](../comm-csp-node-and-ground-gateway-v1/README.md)

This evidence newly proves command/event/channel TT&C with COMM participating on `subsystem.local:can1` over the shared SocketCAN carrier. It keeps file/downlink, RF, no-preamble behavior, dual-bus redundancy, independent COMM hardware, and ScenarioBridge/pass automation as future work.

## Verification

Closeout checks:

| Step | Command | Result |
|---|---|---|
| Stage 0 EPS/ADCS health | `OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_RESERVED_CAN_DEVICE=can1 bash scripts/run_shared_canfd_eps_adcs_health_probe.sh` | PASS |
| formal physical COMM SocketCAN TT&C probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_ttc_probe.sh` | PASS |
| fresh local gate | `bash scripts/run_verification_ci.sh build-artifacts/comm-csp-socketcan-participation-v1-closeout` | PASS |
| fresh-build formal physical probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 OBC_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0 SUBSYSTEM_SIM_COMM_CAN_DEVICE=can1 bash scripts/run_comm_csp_socketcan_ttc_probe.sh` | PASS |
