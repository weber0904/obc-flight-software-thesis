# ttc-over-comm-lab-serial-ingress-v1 Evidence

## Scope

This record covers the first governed physical lab serial ingress path that replaces the hosted PTY serial-ingress stand-in with the current macOS-to-`subsystem.local` RS-485/UART wiring.

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
```

The proof keeps the existing COMM contract unchanged:

- COMM node id: `4`
- service `30`: `UPLINK_POLL`
- service `31`: `DOWNLINK_WRITE`
- service `32`: `LINK_STATUS`
- stock F' framing and stock `fprime-gds`
- `subsystem.local` native build workflow, not Docker/cross-compile

## Not Covered

- full physical lab serial TT&C as a registered path
- fully bounded `fprime-cli events` and `fprime-cli channels` downlink proof
- RF or real radio behavior
- file/downlink behavior
- target OBC migration
- COMM shared CAN FD participation
- ScenarioBridge, `ground_pass_open`, or `link_available`
- clean no-preamble first-byte behavior on the current physical UART link
- replacing the hosted OBC with target-side OBC hardware

## Implemented Probe Entry Point

- `scripts/run_comm_lab_serial_ingress_probe.sh`
- `ground_ttc_gateway --serial-tx-preamble-lines <count> --serial-tx-preamble-delay-ms <milliseconds>`

Default physical command:

```bash
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 \
PREPARE_SUBSYSTEM_WORKSPACE=0 \
bash scripts/run_comm_lab_serial_ingress_probe.sh
```

The probe is staged:

- Stage 0: optional physical prerequisite probes. This final run skipped the strict no-preamble UART preflight because current hardware diagnostics showed the physical link needs acquisition preamble/settling for reliable useful frames.
- Stage 1: bounded command uplink ingress to OBC readback.
- Stage 2: `fprime-cli events` and `fprime-cli channels` visibility after Stage 1 succeeds.

## Final Physical Lab Serial Verdict

Verdict: `PASS` for physical lab serial uplink ingress.

The final post-fresh-build probe did not satisfy the stricter Stage 2 full-TT&C condition, so this evidence does **not** register full physical lab serial TT&C. It registers the bounded uplink ingress boundary: ground commands traverse the physical lab serial link into `subsystem.local` COMM node `4` and produce final OBC readback.

Final passing run:

| Field | Value |
|---|---|
| date | `2026-04-30` |
| command | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 PREPARE_SUBSYSTEM_WORKSPACE=0 bash scripts/run_comm_lab_serial_ingress_probe.sh` |
| formal verdict | `uplink-ingress` |
| log directory | `/tmp/obc-comm-lab-serial-ingress.Ke0iqy` |
| host serial device | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` |
| subsystem serial device | `/dev/serial0` |
| baudrate | `115200` |
| COMM node | `4` |
| local CSP hub host | `127.0.0.1` |
| remote CSP hub host | `<private-lab-host>` |
| CSP hub ports | `56660` / `57660` |
| GDS ports | `50360` / `50361` |
| radio port | `17260` |
| gateway serial TX preamble | `20` lines, `500 ms` delay |
| startup delay before commands | `12 s` |
| bounded command attempts | `6` |

Observed probe verdict:

```text
comm-lab-serial-ingress-probe: PASS
formal-verdict=uplink-ingress
stage0-uart-preflight=SKIPPED
stage0-acquisition=SKIPPED
stage1-uplink-ingress=PASS
stage2-downlink=STOPPED
```

## Stage 1 Uplink Ingress Proof

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
groundLink connected=yes tx=22680 rx=986 txErr=7 rxErr=17
```

The run did not become clean immediately. Earlier readbacks in the same run showed one side changing before the other, link transitions, and increasing `txErr/rxErr`. The probe uses an explicit gateway acquisition preamble plus bounded command retries, and alternates command order so a pending command is not always the first useful binary frame after an idle gap. The formal Stage 1 proof is therefore scoped to bounded command ingress after acquisition/retry behavior, not to first-frame-clean serial behavior.

## Stage 2 Downlink / Full TT&C Attempt

The same final run observed partial command-event visibility through `fprime-cli events`:

```text
OpCodeDispatched : Opcode 0x10033001 dispatched to port 11
OpCodeCompleted : Opcode 0x10033001 completed
OpCodeCompleted : Opcode 0x10034000 completed
```

It also observed telemetry through `fprime-cli channels`:

```text
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 989
```

Stage 2 did not satisfy the script's full-TT&C condition because the final post-fresh-build run did not observe the full bounded event set. The downlink result is therefore diagnostic and `STOPPED`; full physical lab serial TT&C is not registered by this change.

An earlier pre-closeout run reached `formal-verdict=bounded-ttc` in `/tmp/obc-comm-lab-serial-ingress.gaIGY6`, but the authoritative post-fresh-build closeout run above is more conservative and governs this evidence.

## Supporting And Adjacent Baselines

Reused baseline:

- Hosted gateway-backed omitted-RF COMM TT&C through PTY serial ingress:
  [docs/test-records/comm-csp-node-and-ground-gateway-v1/README.md](../comm-csp-node-and-ground-gateway-v1/README.md)

Supporting physical UART evidence:

- macOS-initiated physical UART request/reply preflight:
  [docs/test-records/subsystem-comm-uart-link-preflight-v1/README.md](../subsystem-comm-uart-link-preflight-v1/README.md)
- subsystem-origin bounded acquisition proof:
  [docs/test-records/comm-lab-serial-acquisition-v1/README.md](../comm-lab-serial-acquisition-v1/README.md)

These records are adjacent support only. This evidence newly proves the gateway-backed physical serial uplink ingress path and keeps PTY stand-in, UART preflight, and subsystem-origin acquisition as separate proof boundaries.

## Verification

Closeout checks:

| Step | Command | Result |
|---|---|---|
| syntax check | `bash -n scripts/run_comm_lab_serial_ingress_probe.sh` | PASS |
| diff whitespace check | `git diff --check` | PASS |
| fresh local gate | `bash scripts/run_verification_ci.sh build-artifacts/ttc-over-comm-lab-serial-ingress-v1-closeout` | PASS |
| hosted PTY regression | `GDS_PORT=50380 GDS_TTS_PORT=50381 CSP_HUB_SUB_PORT=56680 CSP_HUB_PUB_PORT=57680 RADIO_PORT=17280 bash scripts/run_comm_csp_ground_gateway_probe.sh` | PASS |
| optional subsystem-origin acquisition support probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 bash scripts/run_comm_lab_serial_acquisition_probe.sh` | STOPPED; decoded `7/8` frames in `/tmp/obc-comm-lab-acq.D6QRtt` |
| focused physical lab serial ingress probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 PREPARE_SUBSYSTEM_WORKSPACE=0 bash scripts/run_comm_lab_serial_ingress_probe.sh` | PASS for `uplink-ingress`; Stage 2 STOPPED |
| post-run remote cleanup check | `ssh operator@subsystem.local "pgrep -af 'comm_csp_node|radio_mock_server|serial_link_probe' || true"` | PASS; no remaining target process other than the check shell |
| post-run host serial cleanup check | `lsof /dev/cu.usbserial-$COMM_SERIAL_DEVICE || true` | PASS; serial device released |
