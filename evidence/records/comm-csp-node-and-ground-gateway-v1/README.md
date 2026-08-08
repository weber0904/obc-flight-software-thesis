# comm-csp-node-and-ground-gateway-v1 Evidence

## Scope

This record governs the first repository-owned hosted omitted-RF TT&C path that routes stock F' framed ground traffic through the `COMM` subsystem instead of using the direct `GDS -> TCP -> OBC` development baseline.

Target architecture for this slice:

- `macOS` runs:
  - headless `fprime-gds`
  - `fprime-cli`
  - `ground_ttc_gateway`
  - hosted `comm_csp_node` as CSP node `4`
  - hosted `OBC` as CSP node `1`
  - governed internal CSP ZMQHUB substrate
  - `pty_pair_bridge` as the lab-side serial-ingress stand-in
- `ground_ttc_gateway` connects northbound to stock `fprime-gds` over TCP and southbound to `PTY_A`
- `comm_csp_node` connects southbound to `PTY_B` and northbound to the governed internal CSP service ports `30`, `31`, and `32`
- `GroundLinkDriver` runs in `comm-csp` mode so `ComFprime` continues to use stock F' framing while the underlying transport path changes

This slice is intentionally bounded:

- It proves the first gateway-backed bidirectional omitted-RF `command/event/tlm` path through `COMM`.
- It keeps the direct `GDS -> TCP -> OBC` baseline as a separate neighboring path.
- It keeps controller-oriented mock/UART external comm baselines separate from the new `COMM` CSP proof.

## Not Covered

- direct `OBC -> GDS` TCP connectivity as the same proof boundary
- split-host `obc.local -> subsystem.local` deployment
- physical SocketCAN / shared CAN FD carrier behavior
- real UART hardware, RF behavior, or vendor-radio semantics
- file/downlink behavior
- independent physical `COMM`, `EPS`, or `ADCS` controllers

## Governing Scripts And Commands

- `bash scripts/run_comm_csp_ground_gateway_probe.sh`
- `bash scripts/run_verification_ci.sh build-artifacts/comm-csp-node-and-ground-gateway-v1-closeout`
- `openspec validate comm-csp-node-and-ground-gateway-v1`
- `openspec validate --specs`

## Fresh Local Gate

Completed on `2026-04-29` before finalizing this record:

| Step | Command | Result |
|---|---|---|
| local gate | `bash scripts/run_verification_ci.sh build-artifacts/comm-csp-node-and-ground-gateway-v1-closeout` | PASS |

This verifies:

- fresh generate/build
- fresh UT generate/build
- repo consistency checks
- component-test baseline checks
- legacy-ZMQ retirement guardrail
- main OpenSpec spec validation

## Successful Hosted Gateway-Backed Validation Run

Date:
- `2026-04-29`

Observed topology parameters from the final governed rerun:

| Setting | Value |
|---|---|
| log directory | `/tmp/obc-comm-csp-ground.mkOHzs` |
| PTY bridge | `PTY_A=/dev/ttys002`, `PTY_B=/dev/ttys003` |
| GDS bind | `0.0.0.0:50160` |
| GDS TTS port | `50161` |
| CSP hub bind | `tcp://0.0.0.0:56320` / `tcp://0.0.0.0:57320` |
| COMM node id | `4` |
| COMM service ports | `30` uplink poll, `31` downlink write, `32` link/status |
| OBC ground-link mode | `comm-csp` |

## Path Segments And Verdict Boundary

The overall newly proven path is:

- `fprime-cli -> fprime-gds -> ground_ttc_gateway -> PTY serial ingress -> comm_csp_node(node 4) -> internal CSP -> OBC`

This evidence keeps the two transport segments separate on purpose:

- lab-side omitted-RF ingress segment:
  - `fprime-cli -> fprime-gds -> ground_ttc_gateway -> PTY_A/PTY_B -> comm_csp_node`
- spacecraft-side internal COMM-to-OBC segment:
  - `comm_csp_node(node 4) <-> OBC(node 1)` over the governed internal CSP substrate using ports `30-32`

This record does **not** claim that the PTY-based serial ingress is equivalent to real UART hardware, and it does **not** reuse the older direct `GDS -> TCP -> OBC` baseline as if it were the same path.

## Bounded Probe Outcomes

Probe behavior:

- the probe started headless `fprime-gds`, `ground_ttc_gateway`, `comm_csp_node`, the hosted OBC stack, and ground-side `fprime-cli` listeners
- it waited for ground-side telemetry to appear before issuing bounded commands
- it dispatched:
  - `OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true`
  - `OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING`
- it collected:
  - ground-side command events through `fprime-cli events`
  - ground-side telemetry through `fprime-cli channels`
  - OBC-side final bounded state readback via operator-shell commands

Observed highlights:

```text
Ground link via COMM CSP node: 4
groundLink mode=comm-csp commNode=4
groundLink connected=yes tx=7108 rx=207 txErr=0 rxErr=1
OpCodeDispatched : Opcode 0x10033001 dispatched to port 11
OpCodeCompleted : Opcode 0x10033001 completed
OpCodeDispatched : Opcode 0x10034000 dispatched to port 7
OpCodeCompleted : Opcode 0x10034000 completed
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 175
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 3633
OBCApp.groundLinkDriver.GROUND_LINK_TX_BYTES ... 5562
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.00,-0.00,0.00) pointingErr=0.00
```

Verdict:

- PASS for the hosted gateway-backed omitted-RF COMM TT&C path.
- This proves the bounded `EPS_SET_PDU(channel=2, enabled=true)` and `ADCS_SET_MODE(POINTING)` flow through:
  - `fprime-cli -> GDS -> gateway -> PTY serial ingress -> COMM node 4 -> internal CSP -> OBC`
- This also proves bounded downlink visibility for:
  - command events through `fprime-cli events`
  - telemetry through `fprime-cli channels` on `GROUND_LINK_TX_BYTES`
- The final OBC-side readback confirmed:
  - `eps ... pdu=7`
  - `adcs mode=POINTING`

## Reused And Adjacent Baselines

- Reused northbound operator surface:
  - stock `fprime-gds`
  - stock F' framing as consumed by `ComFprime`
- Separate neighboring baseline, not redefined by this record:
  - [evidence/records/gds-ground-integration-v1/README.md](../gds-ground-integration-v1/README.md)
- Separate controller-oriented external comm baselines, not interchangeable with this proof:
  - [evidence/records/comm-subsystem-v1/README.md](../comm-subsystem-v1/README.md)
  - [evidence/records/rpi-csp-comm-baseline-validation-v1/README.md](../rpi-csp-comm-baseline-validation-v1/README.md)
- Separate physical internal CSP carrier proof, not revalidated here:
  - [evidence/records/shared-canfd-csp-bus-foundation-v1/README.md](../shared-canfd-csp-bus-foundation-v1/README.md)

## Final Validation Commands

| Step | Command | Result |
|---|---|---|
| focused hosted probe | `bash scripts/run_comm_csp_ground_gateway_probe.sh` | PASS |
| OpenSpec change validate | `openspec validate comm-csp-node-and-ground-gateway-v1` | PASS |
| OpenSpec spec validate | `openspec validate --specs` | PASS |
