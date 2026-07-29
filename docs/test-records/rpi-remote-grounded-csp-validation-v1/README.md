# rpi-remote-grounded-csp-validation-v1 Evidence

## Scope

This record captures the governed remote topology where:

- Raspberry Pi runs only the `OBC` process as libcsp node `1`
- macOS runs `csp_zmqproxy`, `eps_simulator` node `2`, `adcs_simulator` node `3`, and headless `fprime-gds`
- `fprime-cli` on macOS dispatches bounded subsystem commands through GDS into the Pi OBC

The goal is to prove two adjacent but distinct paths:

1. `Pi OBC -> remote macOS EPS/ADCS simulators` internal CSP reachability
2. `macOS fprime-cli -> GDS -> Pi OBC -> remote EPS/ADCS simulators` bounded subsystem command flow

## Not Covered

- external comm / radio path acceptance
- GPS live UART
- RF behavior
- real EPS or ADCS hardware
- future `UART / CAN / RS485` physical-bus behavior

The probe used `COMM_MODE=tcp` to keep external comm out of scope. The resulting `UART_ERROR` / `radio unavailable` lines on the Pi target are expected noise for this change and are not part of the acceptance criteria.

## Governing Script

- `bash scripts/run_rpi_remote_csp_gds_probe.sh`

## Successful Validation Run

Date:
- `2026-04-11`

Topology parameters observed from the successful run:

| Setting | Value |
|---|---|
| `REMOTE_HOST` | `<private-lab-host>` |
| `CSP_TRANSPORT` | `zmqhub` |
| `CSP_HUB_SUB_PORT` | `56059` |
| `CSP_HUB_PUB_PORT` | `57059` |
| `GDS_PORT` | `51059` |
| `GDS_TTS_PORT` | `51060` |
| EPS node id | `2` |
| ADCS node id | `3` |

Host-side stack log excerpt:

```text
Starting remote CSP + GDS host stack
  CSP transport : zmqhub
  CSP bind      : tcp://0.0.0.0:56059 / tcp://0.0.0.0:57059
  GDS bind      : 0.0.0.0:51059 (tts 51060)
  EPS node      : 2
  ADCS node     : 3
Subscriber task listening on tcp://0.0.0.0:56059
Publisher task listening on tcp://0.0.0.0:57059
```

## Path A: Remote Internal CSP Reachability

Probe behavior:

- Pi OBC launched with `--gds-port 0`
- probe sent:
  - `status`
  - `csp ping 2`
  - `csp ping 3`
  - `eps get`
  - `adcs get`

Pi-side observed highlights:

```text
ground enabled=no
csp initialized=yes node=1 tx=49 rx=95 errors=6
csp ping response=0 success=yes
csp ping response=0 success=yes
eps soc=76.00 vbat=8.06 tempBat=25.00 pdu=3
adcs mode=DETUMBLE omega=(0.00,-0.00,0.00) pointingErr=0.00
```

Verdict:

- PASS for the remote internal CSP path.
- This proves Pi OBC node `1` can reach remote EPS node `2` and ADCS node `3` over the governed ZMQHUB/TCP/IP development carrier.
- This section does not prove any GDS command path.

## Path B: GDS-Driven Remote Subsystem Command Flow

Probe behavior:

- Pi OBC launched against remote GDS host `<private-lab-host>:51059`
- host-side `fprime-cli` sent:
  - `OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true`
  - `OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING`
- Pi-side probe then inspected `eps get` and `adcs get`

Pi-side observed highlights:

```text
Connected to <private-lab-host>:51059 as a tcp client
Ground link target: <private-lab-host>:51059
EVENT: (cmdDisp) OpCodeDispatched : Opcode 0x10033001 dispatched to port 11
EVENT: (cmdDisp) OpCodeCompleted : Opcode 0x10033001 completed
EVENT: (cmdDisp) OpCodeDispatched : Opcode 0x10034000 dispatched to port 7
EVENT: (cmdDisp) OpCodeCompleted : Opcode 0x10034000 completed
ground enabled=yes target=<private-lab-host>:51059
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.00,-0.00,0.00) pointingErr=0.00
```

Verdict:

- PASS for the bounded target-side ground-driven subsystem command path.
- This proves the specific `EPS_SET_PDU(channel=2, enabled=true)` and `ADCS_SET_MODE(POINTING)` flow through `fprime-cli -> GDS -> Pi OBC -> remote simulators`.
- This does not imply that every possible GDS command path is already proven.

## Command Summary

| Step | Command | Result |
|---|---|---|
| remote bounded probe | `bash scripts/run_rpi_remote_csp_gds_probe.sh` | PASS |

## Final Verdict

- PASS for `rpi-remote-grounded-csp-validation-v1`.
- The repository now has a governed proof for the original intended remote topology: Pi-hosted OBC, macOS-hosted CSP hub + simulators, and a bounded ground-driven subsystem command path through GDS.
- The proof remains explicitly scoped to a remote development carrier over ZMQHUB/TCP/IP and does not claim future physical-wire or real-hardware equivalence.
