# shared-canfd-csp-bus-foundation-v1 Evidence

## Scope

This record governs the first physical internal CSP carrier path that moves `EPS` and `ADCS` off the hosted `zmqhub` development carrier and onto a shared **CAN FD-capable SocketCAN bus**.

Target architecture for this slice:

- `macOS` runs headless `fprime-gds` only
- `obc.local` runs `OBC` node `1` on a single active CAN interface
- `subsystem.local` runs:
  - `eps_simulator` node `2`
  - `adcs_simulator` node `3`
  - one active CAN interface shared by those logical subsystem nodes
  - one reserved second CAN interface kept off the active bus and self-tested separately

This slice is intentionally bounded:

- It proves a governed internal physical carrier change for logical CSP traffic.
- It does **not** prove independent physical EPS and ADCS controllers.
- It does **not** prove COMM participation, omitted-RF TT&C, or dual-bus redundancy.

## Not Covered

- `COMM` subsystem traffic
- omitted-RF gateway or RF behavior
- independent physical `EPS` and `ADCS` controllers
- dual-bus redundancy
- CAN FD large-payload or BRS behavior inside libcsp
- GPS behavior beyond the already-governed live UART path
- root-owned automation of `/boot/firmware/config.txt` or `ip link`

## Hardware And Wiring Baseline

### OBC active CAN endpoint

`obc.local` keeps the already-validated MCP2518FD Pro single-channel SPI wiring:

| Signal | OBC Pi connection |
|---|---|
| `3V3` | `3V3` |
| `5V` | `5V` |
| `GND` | `GND` |
| `SCLK` | `SPI0 SCLK` |
| `MOSI` | `SPI0 MOSI` |
| `MISO` | `SPI0 MISO` |
| `CS` | `CE0` |
| `INT` | `GPIO25` |

### Subsystem CAN channels

`subsystem.local` keeps the existing 2-channel CAN HAT wiring:

| Channel role | SPI binding | Interrupt | Runtime rule |
|---|---|---|---|
| primary shared-bus channel | `spi0-0` | `GPIO25` | used for `EPS` + `ADCS` logical-node traffic |
| reserved channel | `spi1-0` | `GPIO24` | kept off the active bus; self-test only |

### Active bus wiring

The v1 active bus is:

- OBC transceiver `CAN_H` -> subsystem primary-channel `CAN_H`
- OBC transceiver `CAN_L` -> subsystem primary-channel `CAN_L`
- shared ground between the two CAN transceivers

Termination:

- one `120Ω` resistor at the OBC end of the active bus
- one `120Ω` resistor at the subsystem primary-channel end of the active bus
- reserved subsystem channel not connected into the active bus

## Manual OS / Driver Prerequisites

The repository does **not** automate these root-required steps in this slice.

### `obc.local`

`/boot/firmware/config.txt`:

```ini
dtparam=spi=on
dtoverlay=mcp251xfd,spi0-0,interrupt=25,oscillator=40000000,speed=1000000
```

### `subsystem.local`

`/boot/firmware/config.txt`:

```ini
dtparam=spi=on
dtoverlay=spi1-3cs
dtoverlay=mcp251xfd,spi0-0,interrupt=25,oscillator=40000000,speed=1000000
dtoverlay=mcp251xfd,spi1-0,interrupt=24,oscillator=40000000,speed=1000000
```

### Packages

Both Pis need:

```bash
sudo apt-get install -y libsocketcan-dev can-utils
```

### Manual bring-up

The runtime assumes Linux CAN interfaces are already configured and `UP`.

Active interfaces must be brought up with:

```bash
sudo ip link set canX up type can bitrate 500000 dbitrate 2000000 fd on
```

## Channel Mapping Rule

`subsystem.local` may not assume `can0` or `can1` equals a specific physical channel.

The governed rule for this slice is:

- identify the primary and reserved devices by `parentdev`
- record that mapping in evidence
- feed launchers explicit `SUBSYSTEM_SIM_CSP_CAN_DEVICE` and `SUBSYSTEM_SIM_RESERVED_CAN_DEVICE`

The helper used by repo-owned scripts is:

```bash
source scripts/_common.sh
obc_can_parentdev canX
```

## Governing Scripts

- `bash scripts/run_verification_ci.sh build-artifacts/shared-canfd-csp-bus-foundation-v1-smoke2`
- `bash scripts/run_ground_gds_only_stack.sh`
- `bash scripts/run_subsystem_sim_remote_can_stack.sh`
- `bash scripts/run_rpi_can_csp_stack.sh`
- `bash scripts/run_shared_canfd_csp_gds_probe.sh`

## Fresh Local Gate

Completed before hardware proof:

| Step | Command | Result |
|---|---|---|
| local gate | `bash scripts/run_verification_ci.sh build-artifacts/shared-canfd-csp-bus-foundation-v1-smoke3` | PASS |

This verifies:

- fresh generate/build
- fresh UT generate/build
- repo consistency checks
- component-test baseline checks
- legacy-ZMQ retirement guardrail
- main OpenSpec spec validation

## Target Linux Build Checkpoint

Completed before the governed hardware probe:

| Host | Command | Result |
|---|---|---|
| `obc.local` | `bash scripts/bootstrap_rpi_workspace.sh` | PASS |
| `subsystem.local` | `bash scripts/bootstrap_subsystem_sim_workspace.sh` | PASS |

This confirms:

- Linux builds compile the `socketcan` backend with `libsocketcan-dev`
- the OBC Linux target build links the governed CSP runtime with SocketCAN support
- the subsystem Linux target build links the EPS/ADCS simulator runtime with SocketCAN support

## Hardware Proof Capture

### 1. Driver Enumeration

```bash
dmesg | egrep -i 'mcp25|can|spi'
ip -details link show type can
```

Observed outcome:

- `obc.local` enumerated one active controller:
  - `can0`, `parentdev spi0.0`
  - `state ERROR-ACTIVE`
  - `bitrate 500000`
  - `dbitrate 2000000`
  - `can <FD,TDC-AUTO>`
- `subsystem.local` enumerated two controllers:
  - active shared-bus candidate `can0`, `parentdev spi0.0`
  - reserved channel `can1`, `parentdev spi1.0`
  - both reported `state ERROR-ACTIVE`
  - both reported `bitrate 500000`, `dbitrate 2000000`, and `can <FD,TDC-AUTO>`

### 2. Primary/Reserved Mapping

```bash
source scripts/_common.sh
obc_can_parentdev canX
```

Observed mapping:

```text
primary=can0 parentdev=spi0.0
reserved=can1 parentdev=spi1.0
```

This slice therefore used:

- `SUBSYSTEM_SIM_CSP_CAN_DEVICE=can0`
- `SUBSYSTEM_SIM_RESERVED_CAN_DEVICE=can1`

The mapping was derived from `parentdev`; it was not accepted from `can0/can1` naming alone.

### 3. Reserved-Channel Isolation

Observed outcome:

- reserved-channel `candump` capture stayed empty during the active-bus governed probe:
  - `reserved-bus.candump.log` size: `0`
- reserved-channel statistics were unchanged before and after the active-bus probe:
  - pre: `RX packets=2`, `TX packets=1`, `errors=0`, `bus-off=0`
  - post: `RX packets=2`, `TX packets=1`, `errors=0`, `bus-off=0`

This is the governed negative evidence for v1:

- the reserved subsystem channel remained enumerated and `UP`
- it did not observe active-bus traffic
- it was not treated as an active `COMM` path

### 4. Active-Bus Functional Probe

```bash
bash scripts/run_shared_canfd_csp_gds_probe.sh
```

Observed governed probe:

```text
shared-canfd-csp-gds-probe: PASS
temp dir: /tmp/obc-shared-canfd-csp.DWeKmW
```

Bounded functional outcomes:

- path A, direct OBC operator-shell checks on the SocketCAN carrier:
  - `csp ping 2` -> `csp ping response=0 success=yes`
  - `csp ping 3` -> `csp ping response=0 success=yes`
  - `eps get` observed `eps soc=76.00 vbat=8.06 tempBat=25.00 pdu=3`
  - `adcs get` observed `adcs mode=DETUMBLE`
- path B, `fprime-cli -> GDS -> OBC -> subsystem` checks on the same carrier:
  - observed two `OpCodeDispatched` and two `OpCodeCompleted` command events
  - observed `EPS_PDU_CHANGE : EPS PDU state changed to 7`
  - observed `ADCS_MODE_CHANGE : ADCS mode changed to POINTING (2)`
  - final bounded OBC-side state readback showed:
    - `eps ... pdu=7`
    - `adcs mode=POINTING`

This proves the physical internal carrier migration while keeping the direct `GDS -> OBC` ground path distinct.

### 5. CAN Health And Traffic Evidence

```bash
ip -details -statistics link show dev <can-device>
candump <active-can-device>
```

Observed outcome:

- active-bus `candump` was non-empty:
  - `active-bus.candump.log` size: `36408`
  - sample frames included repeated `4`-byte and `8`-byte traffic records on `can0`
- subsystem active channel stayed healthy:
  - pre: `ERROR-ACTIVE`, `bus-off=0`, `RX packets=3`, `TX packets=0`
  - post: `ERROR-ACTIVE`, `bus-off=0`, `RX packets=1467`, `TX packets=974`
- OBC active channel stayed healthy:
  - pre: `ERROR-ACTIVE`, `bus-off=0`, `RX packets=3`, `TX packets=3`
  - post: `ERROR-ACTIVE`, `bus-off=0`, `RX packets=1467`, `TX packets=493`
- no interface showed `error-passive` or `bus-off` growth during the governed run

Capability vs observed frame behavior:

- this bus was configured as **CAN FD-capable**:
  - `fd on`
  - `dbitrate 2000000`
  - `can <FD,TDC-AUTO>`
- this evidence does **not** claim that libcsp emitted larger CAN FD payloads or BRS-marked frames
- the observed `candump` traffic in this v1 proof consisted of bounded `4`-byte and `8`-byte frame records on the active bus

## Governing Artifacts

Captured governed artifacts for this run:

- parentdev mapping:
  - `/tmp/obc-shared-canfd-csp.DWeKmW/subsystem-parentdev.log`
- active-bus `candump`:
  - `/tmp/obc-shared-canfd-csp.DWeKmW/active-bus.candump.log`
- reserved-channel `candump`:
  - `/tmp/obc-shared-canfd-csp.DWeKmW/reserved-bus.candump.log`
- subsystem pre/post stats:
  - `/tmp/obc-shared-canfd-csp.DWeKmW/subsystem-pre-stats.log`
  - `/tmp/obc-shared-canfd-csp.DWeKmW/subsystem-post-stats.log`
- OBC pre/post stats:
  - `/tmp/obc-shared-canfd-csp.DWeKmW/obc-pre-stats.log`
  - `/tmp/obc-shared-canfd-csp.DWeKmW/obc-post-stats.log`

## Acceptance Boundary

This slice now satisfies all required acceptance conditions:

- physical active-bus wiring captured
- `parentdev` mapping captured
- reserved-channel isolation captured
- governed `run_shared_canfd_csp_gds_probe.sh` pass result captured
- post-run CAN health/statistics captured
