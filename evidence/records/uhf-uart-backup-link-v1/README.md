# uhf-uart-backup-link-v1 Evidence

## Scope

This record proves the first hosted UHF UART backup/beacon path through the explicit UHF COMM simulator identity:

```text
bounded TT&C:
fprime-cli
  -> stock fprime-gds TCP endpoint
  -> ground_ttc_gateway --link-identity uhf
  -> hosted PTY serial UHF stand-in
  -> uhf_comm_csp_node node 6
  -> governed internal CSP substrate
  -> hosted OBC node 1

beacon:
OBC BeaconPublisher
  -> UHF CSP beacon side channel
  -> uhf_comm_csp_node node 6
  -> hosted PTY serial beacon endpoint
  -> capture/decode
```

The proof uses `uhf_comm_csp_node` as CSP node `6`; it is not direct `GDS -> TCP -> OBC`, generic node `4`, or S-band node `5` evidence. Hosted PTY serial stands in for the future macOS UART/USB/RS485 lab segment.

Newly proven scope:

- bounded command/event/channel ingress through `fprime-cli -> GDS -> ground_ttc_gateway(link=uhf serial) -> hosted PTY serial -> uhf_comm_csp_node(node 6) -> OBC`
- BeaconV1 capture/decode through the UHF node-6 beacon side channel
- explicit UHF executable identity, gateway link identity, CSP node, and hosted serial endpoints in probe output and logs
- node-6 beacon evidence remains separate from command ingress evidence

## Not Covered

- S-band TCP behavior
- direct `GDS -> TCP -> OBC`
- generic `comm_csp_node` node `4` behavior as UHF evidence
- full UHF command authority
- autonomous failover policy
- file/downlink or arbitrary downlink
- CCSDS behavior
- RF behavior or real radio behavior
- reliable transfer, packet-loss recovery, NACK/ARQ, or retransmission behavior
- target hardware behavior
- Raspberry Pi deployment
- physical USB serial hardware or physical RS485 electrical behavior

## Implemented Entry Points

- `uhf_comm_csp_node --serial-device <path> --node-id 6 --beacon-serial-device <path>`
- `ground_ttc_gateway --serial-device <path> --link-identity uhf`
- `OBC --uhf-beacon-csp-node 6`
- `UHF_BEACON_CSP_NODE=6 bash scripts/run_dev_stack.sh`
- `scripts/run_uhf_uart_backup_link_probe.sh`

## Hosted UHF Verdict

Verdict: `PASS` for bounded hosted UHF command/event/channel ingress and UHF node-6 BeaconV1 side-channel capture/decode.

Initial passing hosted run:

| Field | Value |
|---|---|
| date | `2026-05-07` |
| command | `bash scripts/run_uhf_uart_backup_link_probe.sh` |
| formal verdict | `bounded-ttc-and-beacon` |
| link mode | `hosted-uhf-pty` |
| log directory | `/tmp/obc-uhf-uart-backup.iOAARW` |
| GDS ports | `50240` / `50241` |
| CSP hub ports | `56421` / `57421` |
| runtime root | `/tmp/uhf-uart-backup-link-runtime` |
| GDS file storage | `/tmp/uhf-uart-backup-link-gds-downlink` |
| COMM executable | `uhf_comm_csp_node` |
| COMM node | `6` |
| COMM interface | `UHFCSP` |
| gateway link identity | `uhf` |
| beacon capture bytes | `108` |

Observed startup and beacon identity lines:

```text
COMM node startup: executable=uhf_comm_csp_node link=uhf node=6 endpoint=serial serial=/dev/ttys015 baudrate=115200 interface=UHFCSP beacon-serial=/dev/ttys011 beacon-baudrate=115200
ground_ttc_gateway startup: link=uhf gds=127.0.0.1:50240 southbound=serial serial=/dev/ttys014 baudrate=115200 preamble-lines=0 preamble-delay-ms=0
UHF beacon CSP sink: node=6 port=33
COMM beacon push: node=6 bytes=108 result=OK beacon-serial=/dev/ttys011 beacon-baudrate=115200
```

Observed probe PASS markers:

```text
uhf-uart-backup-link-probe: PASS
formal-verdict=bounded-ttc-and-beacon
link-mode=hosted-uhf-pty
comm-node=6
```

## Bounded TT&C Ingress

The probe sent controlled commands through the UHF hosted serial through-COMM path:

```text
OBCApp.epsBridge.EPS_SET_PDU --arguments 2 true
OBCApp.adcsBridge.ADCS_SET_MODE --arguments POINTING
```

Observed OBC readback confirmed the command effects and selected COMM node:

```text
Ground link via COMM CSP node: 6
groundLink mode=comm-csp commNode=6
eps soc=76.00 vbat=8.06 tempBat=25.50 pdu=7
adcs mode=POINTING omega=(0.04,-0.03,0.02) pointingErr=0.00
groundLink connected=yes tx=13903 rx=423 txErr=0 rxErr=2
```

The run also observed command dispatch/completion events and nonzero `GROUND_LINK_TX_BYTES` telemetry through `fprime-cli`.

## Beacon Capture

The hosted UHF beacon path captured one BeaconV1 frame from the node-6 side channel and decoded it.

| Artifact | Path | Bytes | SHA-256 |
|---|---|---:|---|
| BeaconV1 binary capture | `/tmp/obc-uhf-uart-backup.iOAARW/uhf-beacon-capture.bin` | `108` | `1921d90575470212bfadc9b622632e1e2ede115eddebae63cbc944d77f144e64` |
| decoded BeaconV1 JSON | `/tmp/obc-uhf-uart-backup.iOAARW/uhf-beacon-decoded.json` | `779` | `60eb925a0f0c6c55a0755412a7ecef3c196a03eb7718acc3a443210f41ce63f3` |

Representative decoded fields:

```json
{
  "type": "BeaconV1",
  "version": 2,
  "sequence": 0,
  "battery_soc": 76.0,
  "battery_voltage": 8.06,
  "battery_current": 0.34,
  "gps_fix_valid": 1,
  "csp_tx_packets": 293,
  "csp_rx_packets": 581
}
```

The frame passed fixed-size, schema-version, CRC, and representative populated-state checks.

Post-gate UHF probe rerun:

| Field | Value |
|---|---|
| date | `2026-05-07` |
| command | `bash scripts/run_uhf_uart_backup_link_probe.sh` |
| log directory | `/tmp/obc-uhf-uart-backup.itndR9` |
| GDS ports | `50240` / `50241` |
| COMM executable | `uhf_comm_csp_node` |
| COMM node | `6` |
| formal verdict | `bounded-ttc-and-beacon` |
| result | PASS |

Post-gate capture artifacts:

| Artifact | Path | Bytes | SHA-256 |
|---|---|---:|---|
| BeaconV1 binary capture | `/tmp/obc-uhf-uart-backup.itndR9/uhf-beacon-capture.bin` | `108` | `4dac095c5f0c53a504ef6aed90d3bf4edf7a431b1936d0f9f16f276880654aab` |
| decoded BeaconV1 JSON | `/tmp/obc-uhf-uart-backup.itndR9/uhf-beacon-decoded.json` | `779` | `88f2b61b71e81756a4ccaafc8e786d3aadd4dbec4b764ff2ea93fd7d92ef299e` |

The post-gate probe started `fprime-cli` event/channel listeners before hosted OBC startup so early `GROUND_LINK_TX_BYTES` telemetry is captured deterministically, while command/event assertions still occur after bounded command injection.

Final post-archive UHF probe rerun:

| Field | Value |
|---|---|
| date | `2026-05-07` |
| command | `bash scripts/run_uhf_uart_backup_link_probe.sh` |
| log directory | `/tmp/obc-uhf-uart-backup.PG5nlX` |
| GDS ports | `50240` / `50241` |
| COMM executable | `uhf_comm_csp_node` |
| COMM node | `6` |
| formal verdict | `bounded-ttc-and-beacon` |
| result | PASS |

Final post-archive capture artifacts:

| Artifact | Path | Bytes | SHA-256 |
|---|---|---:|---|
| BeaconV1 binary capture | `/tmp/obc-uhf-uart-backup.PG5nlX/uhf-beacon-capture.bin` | `108` | `8120068b23acfb0611a65802dd8c3a17e682c18f42dee4aa9e80a46fb9cf33db` |
| decoded BeaconV1 JSON | `/tmp/obc-uhf-uart-backup.PG5nlX/uhf-beacon-decoded.json` | `779` | `8c42c6c5014487223cf4c40dc64e895811280620743d438d78494ceb7dcb3366` |

## Verification

Closeout checks:

| Step | Command | Result |
|---|---|---|
| native build | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build` | PASS |
| unit-test build | `PATH="$PWD/fprime-venv/bin:$PATH" fprime-util build --ut` | PASS |
| focused model unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_sim_model_unit_test` | PASS |
| focused groundlink unit test | `./build-fprime-automatic-native-ut/bin/Darwin/comm_groundlink_unit_test` | PASS |
| focused beacon component unit test | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_BeaconPublisher_ut_exe` | PASS |
| focused CSP bridge unit test | `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CspBridge_ut_exe` | PASS |
| initial hosted UHF UART backup/beacon probe | `bash scripts/run_uhf_uart_backup_link_probe.sh` | PASS |
| hosted dual-link identity/coexistence regression | `bash scripts/run_comm_dual_link_sim_foundation_probe.sh` | PASS |
| hosted S-band TCP regression | `bash scripts/run_sband_tcp_ground_link_probe.sh` | PASS |
| hardened generic COMM ground gateway probe | `bash scripts/run_comm_csp_ground_gateway_probe.sh` | PASS |
| full verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-uhf-uart-backup-link-v1` | PASS |
| post-gate hosted UHF UART backup/beacon probe | `bash scripts/run_uhf_uart_backup_link_probe.sh` | PASS |
| OpenSpec change validation | `openspec validate uhf-uart-backup-link-v1` | PASS |
| OpenSpec baseline spec validation | `openspec validate --specs` | PASS |
| OpenSpec archive | `openspec archive uhf-uart-backup-link-v1 --yes` | PASS |
| reconciliation Markdown regeneration | `python3 scripts/generate_reconciliation_matrix_md.py` | PASS |
| post-archive repo consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| post-archive OpenSpec baseline spec validation | `openspec validate --specs` | PASS |
| post-archive full verification gate | `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-uhf-uart-backup-link-v1-post-archive` | PASS |
| final post-archive hosted UHF UART backup/beacon probe | `bash scripts/run_uhf_uart_backup_link_probe.sh` | PASS |
| review-fix repo consistency | `python3 scripts/check_repo_consistency.py` | PASS |
| review-fix OpenSpec baseline spec validation | `openspec validate --specs` | PASS |
