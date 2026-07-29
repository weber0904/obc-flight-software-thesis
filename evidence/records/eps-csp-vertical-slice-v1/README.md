# Test Record: EPS CSP Vertical Slice v1

- Date: 2026-04-10
- Change: `eps-csp-vertical-slice-v1`
- Layer: L3 hosted integration
- Environment: macOS hosted build, `feature/libcsp-internal-network-base`

## Scope

This record proves the hosted EPS business-traffic path over the internal libcsp substrate.

Newly proven path:

- `csp_zmqproxy` hosted internal CSP hub
- OBC/client node `1`
- EPS simulator node `2`
- EPS application services on ports `10..13`
- `EPS_GET_STATUS` equivalent status request/reply
- `EPS_SET_PDU` equivalent PDU request/reply
- EPS reset request/reply

Reused path:

- Hosted internal libcsp foundation path from [`internal-csp-foundation-v1`](../internal-csp-foundation-v1/README.md)

Not proven by this record:

- ADCS migration to libcsp
- direct `OBC -> GDS` TCP adapter behavior
- `fprime-cli -> GDS` command/uplink behavior
- external comm / UART / transparent framed transport
- GPS fake/replay or live UART behavior
- real EPS hardware behavior

## Configuration

| Item | Value |
|---|---|
| CSP hub host | `127.0.0.1` |
| CSP hub subscribe port | `56200` |
| CSP hub publish port | `57200` |
| CSP proxy bind host | `0.0.0.0` |
| OBC/client CSP node | `1` |
| EPS CSP node | `2` |
| EPS application ports | `10` status, `11` PDU, `12` heater/config, `13` reset |

Ports `0..3` are reserved by libcsp for built-in management services and are not used for EPS application traffic.

## Commands

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH fprime-util build --ut
```

Result: Pass.

```bash
bash scripts/run_eps_csp_integration.sh
```

Observed summary:

```text
eps_csp_integration_test: node=2 soc=76 pdu=0x3 tx=5 rx=10
```

Result: Pass.

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH ctest --test-dir build-fprime-automatic-native-ut -R "OBC_Components_EpsBridge_ut_exe|eps_csp_integration_test" --output-on-failure
```

Observed summary:

```text
100% tests passed, 0 tests failed out of 2
```

Result: Pass.

## Notes

- The first implementation attempt exposed two libcsp integration constraints:
  - EPS application service ports must not occupy libcsp reserved ports `0..3`.
  - simulator application dispatch must inspect `packet->id.dport`, matching libcsp service-handler behavior.
- The legacy direct-ZMQ EPS transport code may remain temporarily for incremental migration support, but it is no longer the active hosted EPS baseline.
- ADCS remains on the legacy direct-ZMQ path until `adcs-csp-vertical-slice-v1`.

## Verdict

Pass. Hosted EPS business traffic is now proven over the internal libcsp substrate.
