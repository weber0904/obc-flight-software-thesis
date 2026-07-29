# ttc-over-comm-lab-serial-downlink-v1 Evidence

## Scope

This record covers the first bounded physical lab serial COMM TT&C proof that includes both:

- bounded physical lab serial command ingress to hosted OBC readback
- bounded ground-side event and telemetry downlink visibility through `fprime-cli`

Newly proven path:

```text
fprime-cli
  -> fprime-gds on macOS
  -> ground_ttc_gateway on macOS
  -> /dev/cu.usbserial-$COMM_SERIAL_DEVICE
  -> physical RS-485/UART lab link
  -> subsystem.local:/dev/serial0
  -> native-built comm_csp_node node 4
  -> CSP ZMQHUB
  -> hosted OBC node 1
  -> COMM downlink write service
  -> physical serial link back through ground_ttc_gateway
  -> fprime-gds
  -> fprime-cli events/channels
```

The proof keeps the existing COMM contract unchanged:

- COMM node id: `4`
- service `30`: `UPLINK_POLL`
- service `31`: `DOWNLINK_WRITE`
- service `32`: `LINK_STATUS`
- stock F' framing and stock `fprime-gds`
- `subsystem.local` native build workflow, not Docker/cross-compile

## Not Covered

- file/downlink behavior over the COMM TT&C path
- RF or real radio behavior
- target OBC migration
- COMM shared CAN FD participation
- ScenarioBridge, `ground_pass_open`, or `link_available`
- clean no-preamble first-byte behavior on the current physical UART link
- replacing the hosted OBC with target-side OBC hardware

## Implemented Probe Entry Point

- `scripts/run_comm_lab_serial_downlink_probe.sh`

Default physical command:

```bash
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 \
PREPARE_SUBSYSTEM_WORKSPACE=0 \
bash scripts/run_comm_lab_serial_downlink_probe.sh
```

The focused downlink probe wraps the staged ingress probe but makes Stage 2 mandatory:

- Stage 1: bounded command uplink ingress to OBC readback.
- Stage 2: `fprime-cli events` and `fprime-cli channels` visibility.
- Formal PASS requires `formal-verdict=bounded-ttc`, `stage1-uplink-ingress=PASS`, and `stage2-downlink=PASS`.

## Final Physical Lab Serial Verdict

Verdict: `PASS` for bounded physical lab serial COMM TT&C command/event/channel scope.

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-01` |
| command | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 PREPARE_SUBSYSTEM_WORKSPACE=0 GDS_PORT=50460 GDS_TTS_PORT=50461 CSP_HUB_SUB_PORT=56460 CSP_HUB_PUB_PORT=57460 RADIO_PORT=17460 bash scripts/run_comm_lab_serial_downlink_probe.sh` |
| formal verdict | `bounded-ttc` |
| log directory | `/tmp/obc-comm-lab-serial-downlink.C3mYHA` |
| host serial device | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` |
| subsystem serial device | `/dev/serial0` |
| baudrate | `115200` |
| COMM node | `4` |
| local CSP hub host | `127.0.0.1` |
| remote CSP hub host | `<private-lab-host>` |
| CSP hub ports | `56460` / `57460` |
| GDS ports | `50460` / `50461` |
| radio port | `17460` |
| gateway serial TX preamble | `20` lines, `500 ms` delay |
| startup delay before commands | `12 s` |
| bounded command attempts | `6` |

Observed probe verdict:

```text
comm-lab-serial-ingress-probe: PASS
formal-verdict=bounded-ttc
stage0-uart-preflight=SKIPPED
stage0-acquisition=SKIPPED
stage1-uplink-ingress=PASS
stage2-downlink=PASS
comm-lab-serial-downlink-probe: PASS
formal-verdict=bounded-ttc
```

## Stage 1 Uplink Ingress Prerequisite

The probe issued bounded ground commands through `fprime-cli`:

```text
OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true
OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING
```

The final OBC operator-shell readback confirmed both state changes after bounded retries:

```text
Ground link via COMM CSP node: 4
groundLink mode=comm-csp commNode=4
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.00,-0.00,0.00) pointingErr=0.00
groundLink connected=yes tx=16842 rx=1068 txErr=5 rxErr=18
```

The run still used the governed acquisition preamble and bounded command retries. This evidence therefore proves bounded TT&C after acquisition/retry behavior; it does not prove clean first-byte physical UART behavior.

## Stage 2 Event And Telemetry Downlink

The final run observed bounded command-event visibility through `fprime-cli events`:

```text
OpCodeDispatched : Opcode 0x10033001 dispatched to port 11
OpCodeCompleted : Opcode 0x10033001 completed
OpCodeDispatched : Opcode 0x10034000 dispatched to port 7
OpCodeCompleted : Opcode 0x10034000 completed
```

It also observed telemetry through `fprime-cli channels`:

```text
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 146
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 383
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 626
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 5573
```

This satisfies the bounded physical lab serial downlink condition for command-event and telemetry scope.

## Supporting And Adjacent Baselines

Reused/supporting baselines:

- Hosted gateway-backed omitted-RF COMM TT&C through PTY serial ingress:
  [docs/test-records/comm-csp-node-and-ground-gateway-v1/README.md](../comm-csp-node-and-ground-gateway-v1/README.md)
- Physical lab serial uplink ingress:
  [docs/test-records/ttc-over-comm-lab-serial-ingress-v1/README.md](../ttc-over-comm-lab-serial-ingress-v1/README.md)
- macOS-initiated physical UART request/reply preflight:
  [docs/test-records/subsystem-comm-uart-link-preflight-v1/README.md](../subsystem-comm-uart-link-preflight-v1/README.md)
- subsystem-origin bounded acquisition proof:
  [docs/test-records/comm-lab-serial-acquisition-v1/README.md](../comm-lab-serial-acquisition-v1/README.md)

This evidence newly proves bounded command/event/channel TT&C over the physical lab serial COMM path. It keeps file/downlink, RF, target OBC, and COMM shared CAN FD as future work.

## Verification

Closeout checks:

| Step | Command | Result |
|---|---|---|
| syntax check | `bash -n scripts/run_comm_lab_serial_downlink_probe.sh` | PASS |
| fresh local gate | `bash scripts/run_verification_ci.sh build-artifacts/ttc-over-comm-lab-serial-downlink-v1-closeout` | PASS |
| hosted PTY regression | `GDS_PORT=50440 GDS_TTS_PORT=50441 CSP_HUB_SUB_PORT=56440 CSP_HUB_PUB_PORT=57440 RADIO_PORT=17440 bash scripts/run_comm_csp_ground_gateway_probe.sh` | PASS |
| focused physical lab serial downlink probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 PREPARE_SUBSYSTEM_WORKSPACE=0 GDS_PORT=50460 GDS_TTS_PORT=50461 CSP_HUB_SUB_PORT=56460 CSP_HUB_PUB_PORT=57460 RADIO_PORT=17460 bash scripts/run_comm_lab_serial_downlink_probe.sh` | PASS |

Diagnostic note:

- An initial hosted PTY regression attempt on default ports stopped because `fprime-gds` reported TTS port `50161` already in use. The same probe passed on explicit alternate ports and is the recorded regression verdict.
