# dual-pi-subsystem-sim-host-validation-v1 Evidence

## Scope

This record captures the governed three-host topology where:

- `macOS` runs `csp_zmqproxy`, headless `fprime-gds`, and `fprime-cli`
- `obc.local` runs only the `OBC` process as libcsp node `1`
- `subsystem.local` runs `eps_simulator` node `2` and `adcs_simulator` node `3`

The goal is to prove three adjacent but distinct paths:

1. `obc.local -> subsystem.local` internal CSP reachability through a macOS-hosted hub
2. `macOS fprime-cli -> GDS -> obc.local -> subsystem.local` bounded subsystem command flow
3. split-host internal CSP plus baseline `/dev/serial0` external comm coexistence on `obc.local`

This record does not replace or redefine the older two-host `Pi OBC -> remote macOS simulators` evidence. It adds a new governed three-host topology.

## Not Covered

- real EPS or ADCS hardware
- GPS live UART
- RF behavior
- vendor-specific radio control-plane validation
- future `CAN / UART / RS485 / Ethernet` physical-bus equivalence
- transparent/framed UART robustness retesting in the new three-host topology

## Governing Scripts

- `bash scripts/bootstrap_subsystem_sim_workspace.sh`
- `bash scripts/run_dual_pi_split_host_gds_probe.sh`
- `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE RPI_COMM_DEVICE=/dev/serial0 bash scripts/run_dual_pi_split_host_comm_probe.sh`

## Host Preparation

Observed preparation on `2026-04-24`:

- `bash scripts/sync_rpi_workspace.sh` and `bash scripts/bootstrap_rpi_workspace.sh` remained valid for `operator@obc.local`
- `bash scripts/bootstrap_subsystem_sim_workspace.sh` succeeded for `operator@subsystem.local:$OBC_HOME/lab/fprime/v0`
- `subsystem.local` required the Linux ZeroMQ development package before the first bootstrap:
  - `sudo apt-get install -y libzmq3-dev`

The subsystem host remains workspace-based only in this slice. No package/install/autostart flow was added for `subsystem.local`.

## Fresh Local Gate

The formal closeout reran the repository local gate before the final probe evidence:

| Step | Command | Result |
|---|---|---|
| local gate | `bash scripts/run_verification_ci.sh build-artifacts/dual-pi-subsystem-sim-host-validation-v1-local` | PASS |

## Successful Three-Host Validation Run

Date:
- `2026-04-24`

Observed topology parameters from the final governed rerun:

| Setting | Main split-host probe | Comm coexistence probe |
|---|---|---|
| `REMOTE_HOST` | `<private-lab-host>` | `<private-lab-host>` |
| `CSP_TRANSPORT` | `zmqhub` | `zmqhub` |
| `CSP_HUB_SUB_PORT` | `56407` | `56723` |
| `CSP_HUB_PUB_PORT` | `57407` | `57723` |
| `GDS_PORT` | `51407` | `51723` |
| `GDS_TTS_PORT` | `51408` | `51724` |
| EPS node id | `2` | `2` |
| ADCS node id | `3` | `3` |
| host serial device | not used | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` |
| OBC serial device | not used | `/dev/serial0` |

## Path A: Three-Host Internal CSP Reachability

Probe behavior:

- `macOS` started the ground stack only:
  - `csp_zmqproxy`
  - headless `fprime-gds`
- `subsystem.local` started only:
  - `eps_simulator --node-id 2`
  - `adcs_simulator --node-id 3`
- `obc.local` started only `OBC`
- the bounded probe sent:
  - `status`
  - `csp ping 2`
  - `csp ping 3`
  - `eps get`
  - `adcs get`

Observed highlights:

```text
CSP bind      : tcp://0.0.0.0:56407 / tcp://0.0.0.0:57407
GDS bind      : 0.0.0.0:51407 (tts 51408)
CSP host      : <private-lab-host>:56407/57407
csp ping response=0 success=yes
csp ping response=0 success=yes
eps soc=76.00 vbat=8.06 tempBat=25.00 pdu=3
adcs mode=DETUMBLE omega=(0.00,-0.00,0.00) pointingErr=0.00
```

Verdict:

- PASS for the three-host internal CSP path.
- This proves `obc.local` node `1` can reach EPS node `2` and ADCS node `3` hosted on `subsystem.local` through the macOS-hosted ZMQHUB/TCP/IP development carrier.
- This section does not prove any GDS command path.

## Path B: Three-Host GDS-Driven Subsystem Command Flow

Probe behavior:

- `obc.local` connected to GDS at `<private-lab-host>:51407`
- `macOS fprime-cli` dispatched:
  - `OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true`
  - `OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING`
- the Pi-side probe then re-read `eps get` and `adcs get`

Observed highlights:

```text
Ground link target: <private-lab-host>:51407
OpCodeDispatched : Opcode 0x10033001 dispatched to port 11
OpCodeCompleted : Opcode 0x10033001 completed
OpCodeDispatched : Opcode 0x10034000 dispatched to port 7
OpCodeCompleted : Opcode 0x10034000 completed
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.00,-0.00,0.00) pointingErr=0.00
```

Verdict:

- PASS for the bounded three-host target-side subsystem command path.
- This proves the specific `EPS_SET_PDU(channel=2, enabled=true)` and `ADCS_SET_MODE(POINTING)` flow through `macOS fprime-cli -> GDS -> obc.local -> subsystem.local`.
- This does not imply that every possible GDS command path is already proven.

## Path C: Three-Host CSP Plus External Comm Coexistence

Probe behavior:

- `macOS` started the CSP hub and host-side `radio_mock_server`
- `subsystem.local` kept providing EPS and ADCS simulator nodes
- `obc.local` ran external comm on `/dev/serial0`
- the bounded probe sent:
  - `status`
  - `csp ping 2`
  - `csp ping 3`
  - `eps get`
  - `adcs get`
  - `radio enable on`
  - `uart raw STATUS`

Observed highlights:

```text
Comm mode: serial (/dev/serial0 @115200)
CSP bind      : tcp://0.0.0.0:56723 / tcp://0.0.0.0:57723
csp ping response=0 success=yes
csp ping response=0 success=yes
eps soc=76.00 vbat=8.06 tempBat=25.00 pdu=3
adcs mode=DETUMBLE omega=(0.00,-0.00,0.00) pointingErr=0.00
radio enable response=0
uart response: STATUS enabled=1 power=10 freq=437000000 temp=32.5 rssi=-68
```

Verdict:

- PASS for split-host internal CSP plus baseline external comm coexistence.
- This proves `obc.local` can keep `/dev/serial0` external comm alive while still reaching remote simulator nodes on `subsystem.local`.
- This does not prove RF behavior, vendor radio-control semantics, or future physical-bus equivalence.

## Command Summary

| Step | Command | Result |
|---|---|---|
| subsystem host bootstrap | `bash scripts/bootstrap_subsystem_sim_workspace.sh` | PASS |
| main three-host probe | `bash scripts/run_dual_pi_split_host_gds_probe.sh` | PASS |
| comm coexistence probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE RPI_COMM_DEVICE=/dev/serial0 bash scripts/run_dual_pi_split_host_comm_probe.sh` | PASS |

## Final Verdict

- PASS for `dual-pi-subsystem-sim-host-validation-v1`.
- The repository now has a governed three-host proof for:
  - `macOS` as ground host and CSP hub host
  - `obc.local` as the OBC target host
  - `subsystem.local` as the subsystem simulator host
- The proof remains explicitly scoped to a software-only development carrier over ZMQHUB/TCP/IP and to bounded baseline external comm coexistence.
