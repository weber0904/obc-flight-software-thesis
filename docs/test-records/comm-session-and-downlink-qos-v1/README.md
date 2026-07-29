# comm-session-and-downlink-qos-v1 Evidence

Date: 2026-05-12.

OpenSpec change: `comm-session-and-downlink-qos-v1`.

## Scope

This record proves the first COMM-owned operational session, link-role, and shared downlink QoS closure on the active `TopCcsds` baseline.

The proof keeps the two hosted ground boundaries distinct:

- S-band command/event/channel and shared-surface arbitration use the default hosted CCSDS path:

```text
fprime-cli
  -> fprime-gds (CCSDS)
  -> ground_ttc_gateway (raw relay)
  -> sband_comm_csp_node node 5
  -> CSP hub
  -> hosted OBC / TopCcsds
```

- UHF backup and UHF-primary-after-switch use the bounded hosted serial CCSDS path:

```text
fprime-cli
  -> fprime-gds (CCSDS)
  -> ground_ttc_gateway (serial southbound)
  -> uhf_comm_csp_node node 6
  -> CSP hub
  -> hosted OBC / TopCcsds
```

This change newly proves:

- `CommController` owns explicit primary command / telemetry / file-link runtime state
- `CommandIngressAuthority` remains the authenticated envelope / session / sequence owner, but COMM now drives ingress role policy per active path
- `S-band primary`, `UHF backup`, and `UHF primary` command-class policy are active runtime behavior rather than detached helper logic
- `HousekeepingArchive` and `DpCatalog` now traverse a COMM-owned shared file/downlink scheduler instead of competing directly for stock `FileDownlink`
- primary-link loss and primary-link switch converge session state and downlink owner state with reviewable events/telemetry
- command-policy proof and file/downlink proof remain separate review surfaces

## Not Covered

- RF behavior
- reliable transfer, ARQ/NACK, retransmission, or CFDP
- arbitrary onboard file downlink
- Raspberry Pi / target hardware closure
- persistent anti-replay state or hardware-backed key storage
- full secure boot chain

## Runtime Owner Boundary

- Public COMM owner:
  - `OBC/Components/CommController/*`
- Auth / lifecycle / sequence owner:
  - `OBC/Components/CommandIngressAuthority/*`
- Internal COMM-owned arbitration helper:
  - `OBC/Components/CommController/CommDownlinkScheduler.*`

The external runtime claim remains a single COMM-owned operational surface even though the PR uses an internal helper to keep the downlink scheduler reviewable.

## Hosted Probe

Repository-owned proof entry point:

```bash
bash scripts/run_comm_session_and_downlink_qos_probe.sh
```

Final passing full run:

| Field | Value |
|---|---|
| verdict | `PASS` |
| formal verdict | `comm-session-and-downlink-qos` |
| log root | `/tmp/comm-session-downlink-qos.OB0Cd6` |
| stage1 runtime | `/tmp/csdq-44f7f6ff` |
| stage2 runtime | `/tmp/csdq-d8be215a` |
| S-band boundary | `CCSDS ground_ttc_gateway + sband_comm_csp_node node 5` |
| UHF boundary | `CCSDS ground_ttc_gateway serial + uhf_comm_csp_node node 6` |

Observed summary markers:

```text
comm-session-and-downlink-qos-probe: PASS
formal-verdict=comm-session-and-downlink-qos
case-sband-auth-high-authority=PASS
case-uhf-backup-low-risk-allow-high-risk-deny=PASS
case-dp-downlink-and-uhf-continuity=PASS
case-pass-stop-observe-only=PASS
case-link-loss-failover=PASS
case-uhf-primary-full-authority-and-file-downlink=PASS
command-proof=separate
file-proof=separate
```

## Command Policy Proof

### 1. S-band primary admits authenticated high-authority traffic

Evidence:

```text
EVENT: COMMAND_SESSION_OPENED : Command session opened ingress 0 identity 1 role 1 session 9301 replaced 0
EVENT: COMM_PASS_START : Comm pass started for 5 sec
```

Ground-side channel proof also showed the default role state before any failover:

```text
OBCApp.commController.COMM_PRIMARY_COMMAND_LINK ... SBAND
OBCApp.commController.COMM_PRIMARY_FILE_LINK ... SBAND
```

### 2. UHF backup keeps low-risk continuity and rejects high-risk mode control

Evidence:

```text
EVENT: COMMAND_SESSION_OPENED : Command session opened ingress 1 identity 2 role 2 session 9401 replaced 0
EVENT: OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
EVENT: COMMAND_AUTHORITY_REJECTED : Command authority rejected opcode 0x10030000 ingress 1 identity 2 role 2 class 2 reason 1 response VALIDATION_ERROR
```

Ground-side channel proof showed the authenticated UHF backup session profile actually became active on the CCSDS-backed UHF path:

```text
OBCApp.commandIngressAuthority.SESSION_ACTIVE_ROLE ... 2
OBCApp.commandIngressAuthority.SESSION_ACTIVE_ID ... 9401
```

### 3. `COMM_STOP_PASS` is observe-only, not an admission gate

Evidence:

```text
EVENT: COMM_PASS_END : Comm pass ended, total passes 1
EVENT: HK_CAPTURE_RECORDED : Housekeeping capture recorded in slot 0 generation 0 count 2
EVENT: HK_CAPTURE_RECORDED : Housekeeping capture recorded in slot 0 generation 0 count 3
EVENT: HK_CAPTURE_RECORDED : Housekeeping capture recorded in slot 0 generation 0 count 4
EVENT: HK_CAPTURE_RECORDED : Housekeeping capture recorded in slot 0 generation 0 count 5
```

Interpretation:

- pass state remained observable
- new post-pass authenticated activity still executed
- command/downlink policy remained driven by link-role and availability state instead of pass state alone

### 4. Link loss revokes the old primary session and converges the role state

Evidence:

```text
EVENT: GROUND_LINK_DOWN : Ground link disconnected mode 2
EVENT: COMM_DOWNLINK_STATE_CHANGED : Comm downlink active owner 0 pending owner 0 reason 6
EVENT: COMMAND_SESSION_REVOKED : Command session revoked ingress 0 identity 1 role 1 session 9301 reason 2
EVENT: COMM_PRIMARY_LINK_CHANGED : Comm primary links command UHF (1) telemetry UHF (1) file UHF (1) reason 2
```

Interpretation:

- the old S-band primary session did not remain silently valid after link loss
- COMM cleared the active downlink owner before switching the primary-link role set

## File / Downlink Proof

### 1. HK and DP now share one deterministic COMM-owned surface

Evidence:

```text
EVENT: COMM_DOWNLINK_STATE_CHANGED : Comm downlink active owner 1 pending owner 0 reason 1
EVENT: OpCodeDispatched : Opcode 0x10030001 dispatched to port 19
EVENT: COMM_DOWNLINK_STATE_CHANGED : Comm downlink active owner 1 pending owner 2 reason 1
EVENT: COMM_DOWNLINK_STATE_CHANGED : Comm downlink active owner 2 pending owner 0 reason 5
EVENT: COMM_DOWNLINK_STATE_CHANGED : Comm downlink active owner 2 pending owner 0 reason 1
```

Interpretation:

- HK first acquired the active owner slot
- UHF backup low-risk command traffic still dispatched while HK owned the file surface
- a later `DpCatalog` transfer became the single pending owner rather than bypassing or preempting HK
- COMM then advanced deterministically from `HK active + DP pending` to `DP active + no pending`

The shared surface also produced a received downlink artifact on the S-band GDS side:

| File | Received path | Bytes | SHA-256 |
|---|---|---:|---|
| `hk-slot-00-g000000.bin` | `/tmp/comm-session-downlink-qos.FEOsJB/sband-ground/gds-files/fprime-downlink/hk-slot-00-g000000.bin` | `1223` | `8ff5a8846ec7f4ccdb9cef43ecf14e0c22e4d7a547f59551775f96acab1598b9` |

### 2. Bounded UHF-primary file/downlink works after primary switch

Stage 2 evidence:

```text
OBCApp.commController.COMM_PRIMARY_LINK_CHANGED ... Comm primary links command UHF telemetry UHF file UHF reason 1
OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED ... ingress 1 identity 2 role 3 session 9402 replaced 0
OpCodeCompleted : Opcode 0x1003a001 completed
fileDownlink SendStarted : Downlink of 252 bytes started from /tmp/comm-session-downlink-qos.FEOsJB/stage2-uhf-primary/runtime/hk/index.csv to hk-index.csv
```

Ground-side event proof on the UHF boundary recorded the role switch and the new UHF-primary session:

```text
OBCApp.commController.COMM_PRIMARY_LINK_CHANGED ... command UHF telemetry UHF file UHF reason 1
OBCApp.commandIngressAuthority.COMMAND_SESSION_OPENED ... ingress 1 identity 2 role 3 session 9402 replaced 0
```

The received file byte-matched the source runtime file:

| File | Source path | Received path | Bytes | SHA-256 |
|---|---|---|---:|---|
| `hk-index.csv` | `/tmp/comm-session-downlink-qos.FEOsJB/stage2-uhf-primary/runtime/hk/index.csv` | `/tmp/comm-session-downlink-qos.FEOsJB/stage2-uhf-primary/uhf-ground/gds-files/fprime-downlink/hk-index.csv` | `252` | `6f1ef96e4f0331340fea0bedd6f4fa5d9b0725b1ffa92c4c1265f9b81b484c83` |

## Verification

Commands run for this change closeout slice:

```text
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommController_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommEgressMux_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_CommandIngressAuthority_ut_exe
bash scripts/run_comm_session_and_downlink_qos_probe.sh
bash scripts/run_verification_ci.sh build-artifacts/comm-session-and-downlink-qos-v1
openspec validate comm-session-and-downlink-qos-v1
openspec validate --specs
```

Result summary:

- `OBC_Components_CommController_ut_exe`: PASS
- `OBC_Components_CommEgressMux_ut_exe`: PASS
- `OBC_Components_CommandIngressAuthority_ut_exe`: PASS
- `bash scripts/run_comm_session_and_downlink_qos_probe.sh`: PASS
- `bash scripts/run_verification_ci.sh build-artifacts/comm-session-and-downlink-qos-v1`: FAIL at `04_build_ut` due pre-existing repository-wide link failure in `housekeeping_snapshot_provider_unit_test`, with the known unresolved symbol `Os::MutexInterface::getDelegate(unsigned char (&)[72])`; this is outside the COMM slice and matches the earlier unrelated full-UT blocker.
- `openspec validate comm-session-and-downlink-qos-v1`: PASS
- `openspec validate --specs`: PASS
