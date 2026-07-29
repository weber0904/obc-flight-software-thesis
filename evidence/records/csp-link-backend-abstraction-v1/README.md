# csp-link-backend-abstraction-v1 Evidence

## Scope

This record captures the runtime-carrier abstraction slice that keeps libcsp as the internal subsystem contract while moving hosted ZMQHUB binding behind an explicit carrier selector.

## Path Under Test

- Internal CSP runtime initialization through `RuntimeConfig.transportKind`
- Current active carrier: `zmqhub`
- Hosted and Pi-local flows continue to use libcsp over the governed ZMQHUB-backed substrate

## Not Covered

- Any new physical-bus behavior such as UART, CAN, or RS485
- New ground-path behavior
- New EPS or ADCS business semantics
- Real subsystem hardware compatibility

## Files Updated

- `simulators/csp/CspRuntime.hpp`
- `simulators/csp/CspRuntime.cpp`
- `simulators/csp/CspRuntimeSmokeMain.cpp`
- `simulators/csp/CspServicePeerMain.cpp`
- `scripts/run_csp_runtime_smoke.sh`
- `scripts/run_eps_csp_integration.sh`
- `scripts/run_adcs_csp_integration.sh`
- `scripts/run_dev_stack.sh`
- `scripts/run_uart_stack.sh`
- `scripts/run_rpi_stack.sh`
- `scripts/run_rpi_uart_stack.sh`
- `scripts/run_rpi_installed_stack.sh`
- `packaging/rpi/launch/run_stack.sh`

## Verification Commands

| Step | Command | Result |
|---|---|---|
| native build | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build` | PASS |
| native ut build | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut` | PASS |
| CSP runtime smoke | `bash scripts/run_csp_runtime_smoke.sh` | PASS |
| EPS CSP integration | `bash scripts/run_eps_csp_integration.sh` | PASS |
| ADCS CSP integration | `bash scripts/run_adcs_csp_integration.sh` | PASS |

## Observed Results

- `RuntimeConfig` now carries `transportKind`, with `zmqhub` as the default governed carrier.
- `LibCspRuntime::init()` now selects a carrier backend instead of embedding ZMQHUB binding directly in the runtime core.
- The existing hosted-local and Pi-local launcher scripts now pass `CSP_TRANSPORT` explicitly.
- The Raspberry Pi installed launcher no longer retains the stale direct-endpoint ADCS assumption; it launches EPS and ADCS simulator nodes by CSP node id.

Representative smoke outputs:

```text
csp_runtime_smoke: node=1 tx=2 rx=2 free=15
eps_csp_integration_test: node=2 soc=76 pdu=0x3 tx=5 rx=10
adcs_csp_integration_test: node=3 mode=2 pointing_error_deg=3.72852 tx=16 rx=32
```

## Verdict

- PASS for the carrier-abstraction foundation slice.
- The repository now treats `ZMQHUB over TCP/IP` as the active development carrier for libcsp rather than as a hard-coded runtime assumption.
- This result does not prove any future physical-bus backend. Those remain separate future work.
