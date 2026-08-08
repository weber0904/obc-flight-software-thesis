# Test Record: ADCS CSP Vertical Slice v1

- Date: 2026-04-10
- Change: `adcs-csp-vertical-slice-v1`
- Layer: L3 hosted integration plus L2 component regression
- Environment: macOS hosted build, `feature/adcs-csp-vertical-slice-v1`

## Scope

This record proves the hosted ADCS business-traffic path over the internal libcsp substrate.

Newly proven path:

- `csp_zmqproxy` hosted internal CSP hub
- OBC/client node `1`
- ADCS simulator node `3`
- ADCS application services on ports `20..23`
- `ADCS_GET_ATTITUDE` equivalent state request/reply
- `ADCS_SET_MODE` equivalent mode request/reply
- `ADCS_SET_TARGET` equivalent target request/reply
- `ADCS_CALIBRATE` equivalent calibration request/reply

Reused path:

- Hosted internal libcsp foundation path from [`internal-csp-foundation-v1`](../internal-csp-foundation-v1/README.md)
- EPS CSP migration pattern from [`eps-csp-vertical-slice-v1`](../eps-csp-vertical-slice-v1/README.md)

Not proven by this record:

- direct `OBC -> GDS` TCP adapter behavior
- `fprime-cli -> GDS` command/uplink behavior
- external comm / UART / transparent framed transport
- GPS fake/replay or live UART behavior
- real ADCS hardware behavior
- final removal of all legacy direct-ZMQ compatibility code

## Configuration

| Item | Value |
|---|---|
| CSP hub host | `127.0.0.1` |
| CSP hub subscribe port | `56300` |
| CSP hub publish port | `57300` |
| CSP proxy bind host | `0.0.0.0` |
| OBC/client CSP node | `1` |
| ADCS CSP node | `3` |
| ADCS application ports | `20` state, `21` mode, `22` target, `23` calibrate |

Ports `0..3` are reserved by libcsp for built-in management services and are not used for ADCS application traffic. The hosted libcsp build-time bind range was raised to cover ADCS application ports `20..23`.

## Commands

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate -f
PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build
PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util generate --ut -f
PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build --ut
```

Result: Pass.

```bash
bash scripts/run_adcs_csp_integration.sh
```

Observed summary:

```text
adcs_csp_integration_test: node=3 mode=2 pointing_error_deg=3.72852 tx=16 rx=32
```

Result: Pass.

```bash
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_AdcsBridge_ut_exe
```

Observed summary:

```text
[==========] 5 tests from 1 test suite ran. (9 ms total)
[  PASSED  ] 5 tests.
```

Result: Pass.

```bash
bash scripts/run_verification_ci.sh build-artifacts/adcs-csp-vertical-slice-v1-final
```

Observed summary:

```text
>>> 01_generate
PASS
>>> 02_build
PASS
>>> 03_generate_ut
PASS
>>> 04_build_ut
PASS
>>> 05_check_all
PASS
>>> 06_report_verification_inventory
PASS
>>> 07_check_repo_consistency
PASS
>>> 08_check_agent_entrypoint
PASS
>>> 09_check_component_test_baseline
PASS
>>> 10_openspec_validate_specs
PASS
```

Result: Pass.

## Notes

- The first ADCS CSP smoke exposed that the previous hosted libcsp bind range was too small for ADCS application ports `20..23`; the repo build now sets that range high enough for the ADCS service set.
- The legacy direct-ZMQ ADCS transport code may remain temporarily for incremental migration support, but it is no longer the active hosted ADCS baseline.
- This record does not revalidate the ground path, external comm path, GPS path, or real ADCS hardware path.

## Verdict

Pass. Hosted ADCS business traffic is now proven over the internal libcsp substrate.
