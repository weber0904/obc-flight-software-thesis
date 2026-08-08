# rpi-csp-comm-baseline-validation-v1 Evidence

## Scope

This record captures the Raspberry Pi target validation path that checks internal CSP simulator reachability and external comm UART behavior in one governed run.

## Environment

- Date: `2026-04-10`
- Workspace: `$REPO_ROOT`
- Branch: `feature/libcsp-internal-network-base`
- Host serial device: `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` when connected
- Target serial device: `/dev/serial0`
- Host / target mode: macOS host plus Raspberry Pi target over SSH

> Historical note (2026-05-27): the bootstrap result below mentions `operator@youjun.local`
> because this record predates the current host-role naming.
> Current governed OBC target references are `operator@obc.local` or `operator@<private-lab-host>`.

## Path Under Test

- Target-side internal CSP:
  - OBC / client node `1`
  - EPS simulator node `2`
  - ADCS simulator node `3`
  - target-local CSP hub ports selected by the probe
- Target-side external comm:
  - `CommController`, `RadioController`, and `UartDriver`
  - comm-owned `/dev/serial0`
  - host-side `radio_mock_server` over explicit serial device

## Not Covered

- Direct `OBC -> GDS` or `fprime-cli -> GDS`.
- GPS live UART.
- RF channel behavior.
- Vendor-specific radio control/configuration plane.
- Real EPS or ADCS hardware.

## Probe Commands

Minimum target baseline:

```bash
HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE \
RPI_COMM_DEVICE=/dev/serial0 \
bash scripts/run_rpi_csp_comm_baseline_probe.sh
```

Optional stronger comm probes when serial hardware is stable:

```bash
bash scripts/run_rpi_endurosat_transparent_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0
bash scripts/run_rpi_endurosat_framed_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0
bash scripts/run_rpi_endurosat_framed_robustness_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0
```

## Verification Commands

| Step | Command | Result |
|---|---|---|
| script syntax | `bash -n scripts/run_rpi_csp_comm_baseline_probe.sh` | PASS |
| Raspberry Pi workspace bootstrap | `bash scripts/bootstrap_rpi_workspace.sh` | PASS; Linux target built on `operator@youjun.local:$OBC_HOME/lab/fprime/v0` |
| target CSP + comm probe | `HOST_SERIAL_DEVICE=/dev/cu.usbserial-$COMM_SERIAL_DEVICE RPI_COMM_DEVICE=/dev/serial0 bash scripts/run_rpi_csp_comm_baseline_probe.sh` | PASS |
| optional transparent probe | `bash scripts/run_rpi_endurosat_transparent_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0` | PASS |
| optional framed probe | `bash scripts/run_rpi_endurosat_framed_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0` | PASS |
| optional robustness probe | `bash scripts/run_rpi_endurosat_framed_robustness_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0` | PASS |

## Expected Observations

- `Comm mode: serial (/dev/serial0 ...)`
- at least two `csp ping response=0 success=yes` lines for EPS and ADCS nodes
- `eps soc=...`
- `adcs mode=...`
- `radio enable response=0`
- `uart response: STATUS enabled=1 ...`

## Verification Summary

- Verdict: PASS for the combined Raspberry Pi internal CSP plus external comm UART baseline.
- The first non-escalated local serial run failed because the Codex sandbox could not open the host serial adapter; the escalated run against `/dev/cu.usbserial-$COMM_SERIAL_DEVICE` succeeded.
- The target probe observed two successful CSP pings, EPS and ADCS state output, serial comm mode on `/dev/serial0`, `radio enable response=0`, and `uart response: STATUS enabled=1 ...`.
- CSP counters included `errors=1` during the run, but the bounded acceptance criteria passed. This is recorded as an observed runtime counter, not a probe failure.

## Probe Highlights

Combined Pi CSP + comm probe:

```text
Comm mode: serial (/dev/serial0 @115200)
csp initialized=yes node=1 tx=4 rx=6 errors=1
eps soc=76.00 vbat=8.06 tempBat=25.00 pdu=3
adcs mode=IDLE omega=(0.12,-0.08,0.06) pointingErr=0.00
csp ping response=0 success=yes
csp ping response=0 success=yes
radio enable response=0
uart response: STATUS enabled=1 power=10 freq=437000000 temp=32.5 rssi=-68
```

Transparent comm probe:

```text
Comm mode: serial (/dev/serial0 @230400)
Radio protocol: transparent-passive
uart response: ENDUROSAT_PING
uart response: S_BAND_FRAME_001
uart response: PAYLOAD-ALPHA-123
```

Framed comm probe:

```text
uart frame response hex: 454E4455524F5341545F50494E47
uart frame response ascii: ENDUROSAT_PING
uart frame response hex: 007E7D414243FF10
uart frame response ascii: .~}ABC..
transparent-framed-echo frame=1 rx-bytes=14 hex=454E4455524F5341545F50494E47
transparent-framed-echo frame=2 rx-bytes=8 hex=007E7D414243FF10
transparent-framed-echo frame=3 rx-bytes=15 hex=0102030405060708090A0B0C0D0E0F
```

Framed robustness probe:

```text
transparent-framed-echo session-end frames=3 reason=max-frames
transparent-framed-echo frame=1 rx-bytes=6 hex=DEADBEEF0011
transparent-framed-echo frame=2 rx-bytes=8 hex=A1B2C3D4E5F60708
transparent-framed-echo frame=3 rx-bytes=8 hex=5566778899AABBCC
uart framed exchange failed
uart framed exchange failed
uart frame response hex: DEADBEEF0011
uart frame response hex: A1B2C3D4E5F60708
```

The two `uart framed exchange failed` lines are expected negative/recovery exercise points in the robustness probe, not final verdict failures.
