# comm-dual-link-sim-foundation-v1 Evidence

## Scope

This record governs the hosted COMM simulator identity/coexistence foundation for three explicit process identities:

- `comm_csp_node`: generic compatibility COMM, default CSP node `4`, link identity `generic`, interface `COMMCSP`
- `sband_comm_csp_node`: hosted S-band COMM simulator foundation, default CSP node `5`, link identity `sband`, interface `SBANDCSP`
- `uhf_comm_csp_node`: hosted UHF COMM simulator foundation, default CSP node `6`, link identity `uhf`, interface `UHFCSP`

All three identities reuse the existing COMM CSP service contract:

- `30 UPLINK_POLL`
- `31 DOWNLINK_WRITE`
- `32 LINK_STATUS`

The foundation probe runs the three identities concurrently on the hosted CSP ZMQHUB substrate with distinct PTY serial stand-ins and distinct logs.

## Not Covered

- complete S-band GDS path behavior
- UHF UART/RS485/USB/macOS backup behavior
- CCSDS behavior
- RF behavior
- reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
- file/downlink behavior
- target hardware behavior
- Raspberry Pi deployment

## Governing Scripts And Commands

- `bash scripts/run_comm_dual_link_sim_foundation_probe.sh`
- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-comm-dual-link-sim-foundation`
- `openspec validate comm-dual-link-sim-foundation`
- `openspec validate --specs`

## Successful Hosted Foundation Probe

Date:
- `2026-05-07`

Observed topology parameters from the post-gate hosted rerun:

| Setting | Value |
|---|---|
| log directory | `/tmp/obc-comm-dual-link-sim.NIIKK9` |
| OBC node | `1` |
| generic COMM executable | `comm_csp_node` |
| generic COMM node | `4` |
| generic COMM interface | `COMMCSP` |
| S-band COMM executable | `sband_comm_csp_node` |
| S-band COMM node | `5` |
| S-band COMM interface | `SBANDCSP` |
| UHF COMM executable | `uhf_comm_csp_node` |
| UHF COMM node | `6` |
| UHF COMM interface | `UHFCSP` |
| COMM service ports | `30` uplink poll, `31` downlink write, `32` link/status |
| OBC ground-link mode | disabled |

Observed startup identity lines:

```text
COMM node startup: executable=comm_csp_node link=generic node=4 serial=/dev/ttys014 baudrate=115200 interface=COMMCSP
COMM node startup: executable=sband_comm_csp_node link=sband node=5 serial=/dev/ttys016 baudrate=115200 interface=SBANDCSP
COMM node startup: executable=uhf_comm_csp_node link=uhf node=6 serial=/dev/ttys018 baudrate=115200 interface=UHFCSP
```

Observed OBC reachability:

```text
csp ping response=0 success=yes
csp ping response=0 success=yes
csp ping response=0 success=yes
```

Observed service probe results:

```text
comm_service_probe: PASS link=generic targetNode=4 localNode=7 rxChunks=1 txChunks=1 rxErrors=0 txErrors=0 probeTx=5 probeRx=18
comm_service_probe: PASS link=sband targetNode=5 localNode=8 rxChunks=1 txChunks=1 rxErrors=0 txErrors=0 probeTx=5 probeRx=22
comm_service_probe: PASS link=uhf targetNode=6 localNode=9 rxChunks=1 txChunks=1 rxErrors=0 txErrors=0 probeTx=5 probeRx=22
```

Verdict:

- PASS for hosted identity/coexistence of generic COMM node `4`, S-band COMM node `5`, and UHF COMM node `6`.
- PASS for OBC CSP ping reachability to all three hosted COMM nodes on the same internal CSP substrate.
- PASS for bounded live service exercise of `UPLINK_POLL`, `DOWNLINK_WRITE`, and `LINK_STATUS` on all three hosted COMM identities.

## Verification Closeout

Completed on `2026-05-07` before finalizing this record:

| Step | Command | Result |
|---|---|---|
| focused model unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_sim_model_unit_test` | PASS |
| focused groundlink unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_groundlink_unit_test` | PASS |
| full local gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-comm-dual-link-sim-foundation` | PASS |
| focused hosted probe after fresh build | `bash scripts/run_comm_dual_link_sim_foundation_probe.sh` | PASS |
| OpenSpec change validation | `openspec validate comm-dual-link-sim-foundation` | PASS |
| OpenSpec spec validation before archive | `openspec validate --specs` | PASS |
| OpenSpec archive | `openspec archive comm-dual-link-sim-foundation --yes` | PASS |
| reconciliation Markdown regeneration | `python3 scripts/generate_reconciliation_matrix_md.py` | PASS |
| repo consistency after archive | `python3 scripts/check_repo_consistency.py` | PASS |
| OpenSpec spec validation after archive | `openspec validate --specs` | PASS |
