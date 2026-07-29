# subsystem-comm-uart-link-preflight-v1 Evidence

## Scope

This record covers the first repository-owned preflight for the physical serial wiring now attached to `subsystem.local`.

The formal proof is intentionally narrow:

- `subsystem.local` starts a native-built `radio_mock_server --mode mock-text` on an explicit subsystem serial device.
- macOS runs `serial_link_probe` against an explicit host serial device.
- The probe performs bounded `STATUS`, `ENABLE 1`, `STATUS` exchanges and requires the final response to include `STATUS enabled=1`.
- macOS is the initiator in the formal repo-owned probe; `subsystem.local` replies on the same serial session after receiving the macOS request.

This change preserves the current subsystem platform baseline: the workspace is synced to `subsystem.local` and built natively on the target. It does not introduce Docker, cross-compilation, sysroots, or installed subsystem bundles.

## Not Covered

- gateway-backed TT&C over this physical serial link
- RF or real radio behavior
- file/downlink behavior
- direct `GDS -> TCP -> OBC` behavior
- target OBC migration
- `subsystem.local` cold-first unsolicited downlink as a clean first frame
- COMM shared CAN FD participation
- `ScenarioBridge`, `ground_pass_open`, or `link_available`

## Implemented Probe Entry Points

- `simulators/comm/SerialLinkProbeMain.cpp`
- `simulators/comm/MockRadioServerMain.cpp`
- `scripts/run_subsystem_comm_uart_link_probe.sh`

The script requires both endpoint paths:

```bash
HOST_SERIAL_DEVICE=/dev/cu.<adapter> \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/<subsystem-tty> \
bash scripts/run_subsystem_comm_uart_link_probe.sh
```

## Fresh Local Gate

Completed on `2026-04-30` after the final probe-script edits and rerun during closeout:

| Step | Command | Result |
|---|---|---|
| syntax check | `bash -n scripts/run_subsystem_comm_uart_link_probe.sh` | PASS |
| local gate | `bash scripts/run_verification_ci.sh build-artifacts/subsystem-comm-uart-link-preflight-v1-closeout` | PASS |

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

## Subsystem Native Build Prep

Completed on `2026-04-30`:

| Step | Command | Result |
|---|---|---|
| sync/bootstrap native workspace | `bash scripts/bootstrap_subsystem_sim_workspace.sh` | PASS |
| sync final UART probe updates | `bash scripts/sync_subsystem_sim_workspace.sh` | PASS |
| rebuild final subsystem serial helpers | `ssh operator@subsystem.local 'cd $OBC_HOME/lab/fprime/v0 && fprime-venv/bin/cmake --build build-fprime-automatic-native --target radio_mock_server serial_link_probe'` | PASS |

Observed target:

```text
Linux subsystem 6.12.75+rpt-rpi-v8 ... aarch64 GNU/Linux
```

The native build produced:

```text
$OBC_HOME/lab/fprime/v0/build-fprime-automatic-native/bin/Linux/serial_link_probe
$OBC_HOME/lab/fprime/v0/build-fprime-automatic-native/bin/Linux/radio_mock_server
```

## Endpoint Inventory

Host macOS serial endpoints observed:

```text
/dev/cu.usbserial-$COMM_SERIAL_DEVICE
/dev/tty.usbserial-$COMM_SERIAL_DEVICE
```

`subsystem.local` serial endpoint inventory before the target reboot:

```text
ls: cannot access '/dev/serial*': No such file or directory
ls: cannot access '/dev/ttyAMA*': No such file or directory
ls: cannot access '/dev/ttyUSB*': No such file or directory
ls: cannot access '/dev/ttyACM*': No such file or directory
```

After disabling serial console/getty and rebooting, the intended subsystem endpoint appeared:

```text
/dev/serial0 -> ttyS0
/dev/ttyS0 owner/group/mode: root:dialout crw-rw----
```

Related target configuration observations:

```text
/boot/firmware/config.txt: enable_uart=1
/boot/firmware/cmdline.txt: console=tty1 ...
/proc/cmdline: ... 8250.nr_uarts=1 ... console=tty1 ...
serial-getty@ttyS0.service: inactive
operator read/write access to /dev/serial0: yes/yes
```

## Focused UART Probe Verdict

Verdict: `LIMITED PASS`.

The governed probe proved bounded physical byte exchange for the macOS-initiated request/reply path between the macOS USB-RS485 adapter and `subsystem.local:/dev/serial0`. It does not prove that `subsystem.local` can initiate a clean first frame to a passive macOS receiver.

Final passing repo-owned probe, rerun after the fresh closeout build:

| Setting | Value |
|---|---|
| command | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 bash scripts/run_subsystem_comm_uart_link_probe.sh` |
| direction | `mac-to-subsystem` |
| host serial device | `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` |
| subsystem serial device | `/dev/serial0` |
| baudrate | `115200` |
| log directory | `/tmp/obc-subsystem-comm-uart.C43DYp` |

Observed repo-owned probe highlights:

```text
subsystem-comm-uart-link-probe: PASS
direction=mac-to-subsystem
initial-status-status: OK
initial-status-response: STATUS enabled=0 power=10 freq=437000000 temp=32.5 rssi=-68
enable-status: OK
enable-response: OK enabled=1 power=10 freq=437000000 temp=32.5 rssi=-68
final-status-status: OK
final-status-response: STATUS enabled=1 power=10 freq=437000000 temp=32.5 rssi=-68
serial-link-probe: PASS
serial-link-probe-stats: connected=yes tx=23 rx=176 txErr=0 rxErr=0
```

Post-run cleanup check:

```text
pgrep -af 'radio_mock_server|serial_link_probe|pi_rs485_recv.py'
```

reported no remaining subsystem serial helper process other than the checking shell itself.

Known-good external cross-check, also macOS-initiated:

```text
mac_rs485_send.py: PASS: 3/3 bidirectional RS-485 rounds completed
pi_rs485_recv.py: PASS: handled 3 bidirectional RS-485 rounds
```

This cross-check includes bytes in both directions inside a Mac-initiated handshake: macOS sends `ping`, `subsystem.local` replies with `ack` and `pi_ping`, and macOS replies with `pi_ack`. It is not evidence that `subsystem.local` can initiate the first useful frame on a cold/passive Mac receiver.

## Subsystem-Initiated Diagnostic Results

Additional diagnostics were run after closeout review because COMM eventually needs both uplink and downlink behavior. These diagnostics are intentionally recorded as negative or partial evidence, not as part of the formal PASS boundary above.

Hardware context:

- Mac USB-RS485 adapter: `UT-890A`
- `subsystem.local` TTL-to-RS485 module: Taiwan Sensor TTL-to-RS485 industrial automatic-direction module
- macOS serial endpoints: `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` and `/dev/tty.usbserial-$COMM_SERIAL_DEVICE`
- subsystem serial endpoint: `/dev/serial0`
- baudrate: `115200`

Observed diagnostic outcomes:

| Test | Result | Conclusion |
|---|---|---|
| macOS `screen /dev/cu.usbserial-$COMM_SERIAL_DEVICE 115200`, subsystem sends `pi-first-0` through `pi-first-9` | PASS; macOS displayed all `pi-first-*` lines in the manual interactive session | Hardware can carry subsystem-origin bytes to macOS under `screen` |
| detached `screen -L` receiver, subsystem sends `pi-screenlog-*` | PARTIAL; early bytes were garbage, later lines such as `pi-screenlog-4` through `pi-screenlog-9` were readable | Cold subsystem-origin traffic can need an acquisition/settling interval before clean text appears |
| pyserial long raw receiver on macOS, subsystem sends `pi-long-*` | PARTIAL; early chunks were binary-looking garbage, later chunks included readable `pi-long-8` through `pi-long-12` before later corruption resumed | pyserial can observe subsystem-origin bytes, but the cold-first stream is not clean enough for a first-frame JSON handshake |
| pyserial Mac responder waits for subsystem `ping#1`, subsystem sends formal JSON handshake without warm-up | FAIL; Mac timed out waiting for `ping#1`, subsystem timed out waiting for `ack#1` | Formal subsystem-initiated first-frame request/reply is not proven |
| pyserial Mac responder waits for subsystem `ping#1`, subsystem sends 12 or 20 warm-up lines before formal JSON handshake | FAIL; Mac still timed out waiting for `ping#1`, subsystem timed out waiting for `ack#1` | The tested warm-up approach did not make the formal reverse handshake reliable |

Diagnostic conclusion:

`subsystem.local` can transmit bytes that macOS can observe under `screen`, and pyserial can observe some subsystem-origin bytes in a long receive window. However, the first useful subsystem-origin frame is not reliably clean enough for the tested strict JSON request/reply protocol. The formal proof boundary for this change therefore remains macOS-initiated request/reply only. A later COMM lab serial ingress or downlink change must handle or separately prove subsystem-origin cold-first/downlink acquisition before claiming unsolicited downlink readiness.

Earlier failed diagnostic runs before final cleanup/fix:

| Baudrate | Extra target handling | Result | Runtime log |
|---|---|---|---|
| `115200` | initial run before getty/permission handling | FAIL: initial `STATUS` timeout | `/tmp/obc-subsystem-comm-uart.pvCqB0` |
| `115200` | stop/restore `serial-getty@ttyS0`, no sudo | FAIL: initial `STATUS` I/O error because the console-owned tty was not readable/writable by `operator` | `/tmp/obc-subsystem-comm-uart.3XgiQG` |
| `115200` | stop/restore `serial-getty@ttyS0`, run probe with `sudo -n` | FAIL: initial `STATUS` timeout | `/tmp/obc-subsystem-comm-uart.QAdpSY` |
| `230400` | stop/restore `serial-getty@ttyS0`, run probe with `sudo -n` | FAIL: initial `STATUS` timeout | `/tmp/obc-subsystem-comm-uart.WknGUg` |
| `115200` | same target handling, but host endpoint changed to `/dev/tty.usbserial-$COMM_SERIAL_DEVICE` | FAIL: initial `STATUS` timeout | latest `/tmp/obc-subsystem-comm-uart.*` from the `/dev/tty.*` retry |

Additional host-only endpoint sanity check:

- local `serial_link_probe --serial-device /dev/tty.usbserial-$COMM_SERIAL_DEVICE --baudrate 115200 --timeout-ms 300` returned `initial-status-status: TIMEOUT`, which means the macOS-side write path completed and then waited for a reply that was not present
- the `/dev/tty.*` full-link retry did not change the subsystem-side timeout verdict

Representative failing subsystem log:

```text
stopping-active-serial-getty: serial-getty@ttyS0.service
initial-status-status: TIMEOUT
Failed initial STATUS exchange
```

The final implementation changed the repo-owned probe to default to the lab-ingress direction, `mac-to-subsystem`, because this is the direction relevant to the next `ground_ttc_gateway -> subsystem.local` ingress step. The old `subsystem-to-mac` direction remains available as `PROBE_DIRECTION=subsystem-to-mac` for diagnostics, but it is not the registered proof boundary for this change and was not shown to pass as a clean cold-first subsystem-origin exchange.

## Handoff

To rerun the proven macOS-initiated preflight:

```bash
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
SUBSYSTEM_SIM_COMM_DEVICE=/dev/serial0 \
bash scripts/run_subsystem_comm_uart_link_probe.sh
```

Expected PASS markers:

```text
serial-link-probe: PASS
final-status-response: STATUS enabled=1 ...
subsystem-comm-uart-link-probe: PASS
```

This focused preflight has a limited pass. The next COMM change may use it only as evidence that macOS can initiate bounded physical serial request/reply traffic into `subsystem.local`. It must not reuse this evidence as proof of clean subsystem-origin cold-first downlink, RF, file/downlink, final TT&C, or COMM shared-bus behavior.

## Claim Audit

Before closing this change, the repository claims were checked and narrowed to avoid overstating the result:

- Proven: macOS-initiated bounded request/reply byte exchange over `/dev/cu.usbserial-$COMM_SERIAL_DEVICE <-> /dev/serial0` at `115200`.
- Proven: subsystem serial console/getty cleanup and device permissions are suitable for this bounded preflight.
- Not proven: clean subsystem-initiated first-frame request/reply to a passive macOS receiver.
- Not proven: TT&C over the physical serial link, event/telemetry downlink, RF, file/downlink, target OBC migration, or COMM shared CAN FD participation.

## Final OpenSpec Validation

| Step | Command | Result |
|---|---|---|
| pre-archive change validate | `openspec validate subsystem-comm-uart-link-preflight-v1` | PASS |
| post-claim-audit specs validate | `openspec validate --specs` | PASS |
| post-claim-audit repo consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| post-claim-audit whitespace check | `git diff --check` | PASS |
