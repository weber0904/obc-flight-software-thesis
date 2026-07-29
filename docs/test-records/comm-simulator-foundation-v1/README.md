# comm-simulator-foundation-v1 Evidence

## Scope

This record governs the hosted `COMM` simulator foundation refactor that introduces a `CommSimModel` business layer behind the existing `CommNodeServer` / `comm_csp_node` shell.

The change proves:

- `COMM` now has an explicit simulator model/server split aligned with the `EPS` and `ADCS` simulator pattern.
- The model owns bounded uplink queue state, drop-new overflow accounting, link state, downlink acceptance/backpressure state, status counters, and model-only fault injection hooks.
- The existing gateway-backed omitted-RF TT&C path remains compatible with the unchanged `COMM` CSP wire contract.

The change preserves:

- `COMM` CSP node id `4`
- `COMM` reserved service range `30-39`
- existing services:
  - `30` `UPLINK_POLL`
  - `31` `DOWNLINK_WRITE`
  - `32` `LINK_STATUS`
- existing request/reply wire layouts for those services

## Not Covered

- RF behavior or real radio control semantics
- file/downlink scope
- custom `fprime-gds` plugin behavior
- direct `GDS -> TCP -> OBC` proof as a new result
- controller-oriented mock, transparent, framed, or historical UART external comm baselines
- physical SocketCAN or shared CAN FD `COMM` participation
- runtime/operator-facing COMM debug or injection controls

## Governing Commands

- `./build-fprime-automatic-native-ut/bin/Darwin/comm_sim_model_unit_test`
- `./build-fprime-automatic-native-ut/bin/Darwin/comm_groundlink_unit_test`
- `bash scripts/run_verification_ci.sh build-artifacts/comm-simulator-foundation-v1-closeout`
- `bash scripts/run_comm_csp_ground_gateway_probe.sh`
- `openspec validate comm-simulator-foundation-v1`
- `openspec validate --specs`

## Fresh Local Gate

Completed on `2026-04-29` before finalizing this record:

| Step | Command | Result |
|---|---|---|
| focused model unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_sim_model_unit_test` | PASS |
| focused groundlink compatibility unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_groundlink_unit_test` | PASS |
| local gate | `bash scripts/run_verification_ci.sh build-artifacts/comm-simulator-foundation-v1-closeout` | PASS |

The full local gate summary recorded:

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

## Model Regression Coverage

`comm_sim_model_unit_test` covers:

- default and reset state
- physical link and forced-disconnect flags
- bounded uplink queue ordering
- `NO_CHUNK` behavior
- drop-new overflow with preserved queued bytes
- overflow reflected in legacy `rxErrors`
- invalid request/version handling
- downlink acceptance
- downlink rejection for backpressure, forced disconnect, and injected I/O error state
- status counter reporting through `LINK_STATUS`

`comm_groundlink_unit_test` remains green and confirms the OBC-side `CommCspGroundLinkBackend` still uses the existing `30-32` service contract.

## Focused Hosted Gateway Compatibility Probe

Final successful run:

| Setting | Value |
|---|---|
| date | `2026-04-29` |
| log directory | `/tmp/obc-comm-csp-ground.7t6QlF` |
| explicit GDS bind | `0.0.0.0:50220` |
| explicit GDS TTS port | `50221` |
| explicit CSP hub ports | `56420` / `57420` |
| explicit radio mock port | `17120` |
| COMM node id | `4` |
| COMM service ports | `30`, `31`, `32` |

Command:

```bash
GDS_PORT=50220 GDS_TTS_PORT=50221 CSP_HUB_SUB_PORT=56420 CSP_HUB_PUB_PORT=57420 RADIO_PORT=17120 bash scripts/run_comm_csp_ground_gateway_probe.sh
```

The first probe attempt used the default dynamic GDS/TTS port range and failed before COMM traffic because `fprime-gds` reported `0.0.0.0:50161` already in use. The successful rerun used explicit alternate ports and did not require code changes.

Observed highlights:

```text
comm-csp-ground-gateway-probe: PASS
Ground link via COMM CSP node: 4
groundLink mode=comm-csp commNode=4
groundLink connected=yes tx=6690 rx=195 txErr=0 rxErr=1
OpCodeDispatched : Opcode 0x10033001 dispatched to port 11
OpCodeCompleted : Opcode 0x10033001 completed
OpCodeDispatched : Opcode 0x10034000 dispatched to port 7
OpCodeCompleted : Opcode 0x10034000 completed
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 4495
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.00,-0.00,0.00) pointingErr=0.00
```

Verdict:

- PASS for gateway-backed path compatibility after the `CommSimModel` refactor.
- This reuses the previously registered hosted gateway-backed `COMM` omitted-RF TT&C path.
- This change does not register a new validation path; it adds simulator foundation coverage and a compatibility rerun against the existing path.

## Final OpenSpec Validation

| Step | Command | Result |
|---|---|---|
| change validate | `openspec validate comm-simulator-foundation-v1` | PASS |
| specs validate | `openspec validate --specs` | PASS |
