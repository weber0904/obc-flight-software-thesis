# reliable-transfer-v1 Evidence

## Scope

This record covers the first bounded reliable-transfer slice for the current
default S-band node-`5` path and the current official HK `.fdp` family only.

Proven in this record:

- `DpCatalog.BUILD_CATALOG` and `DpCatalog.START_XMIT_CATALOG` still select the
  current official HK `.fdp` products
- `CommController` owns admission and routes selected whole-file official
  `.fdp` requests into the bounded reliable-transfer helper
- the helper uses node-`5` sidecar services `37` and `38`
- the node-`5` receiver promotes a final artifact only after final transfer
  completion
- hosted proof distinguishes happy path, resend-before-success, and bounded
  retry exhaustion
- target/lab default node-`5` proof reaches the same happy-path bounded
  official `.fdp` verdict through the service-managed SocketCAN baseline

Not proven here:

- UHF reliable transfer
- dual-link arbitration
- RF or over-the-air behavior
- restart-persistent resume
- broad CFDP adoption

## Implemented Probe Entry Points

- `scripts/run_comm_reliable_transfer_hosted_probe.sh`
- `scripts/run_comm_csp_socketcan_reliable_transfer_probe.sh`

Hosted happy-path command:

```bash
bash scripts/run_comm_reliable_transfer_hosted_probe.sh
```

Hosted degraded resend command:

```bash
PROBE_MODE=ack-loss bash scripts/run_comm_reliable_transfer_hosted_probe.sh
```

Hosted bounded failure command:

```bash
PROBE_MODE=retry-exhausted bash scripts/run_comm_reliable_transfer_hosted_probe.sh
```

Target/lab node-`5` wrapper entry point:

```bash
bash scripts/run_comm_csp_socketcan_reliable_transfer_probe.sh
```

## Hosted Happy-Path Verdict

Verdict: `PASS` for hosted default node-`5` official HK `.fdp` reliable
transfer.

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-26` |
| command | `bash scripts/run_comm_reliable_transfer_hosted_probe.sh` |
| formal verdict | `reliable-transfer` |
| probe mode | `happy` |
| result | `success` |
| COMM node | `5` |
| runtime root | `/tmp/comm-reliable-transfer-runtime` |
| RT output dir | `/tmp/comm-reliable-transfer-hosted.hIcCeL/rt-output` |
| GDS file storage | `/tmp/comm-reliable-transfer-hosted.hIcCeL/gds-downlink` |
| logs | `/tmp/comm-reliable-transfer-hosted.hIcCeL` |

Observed PASS markers:

```text
comm-reliable-transfer-hosted-probe: PASS
formal-verdict=reliable-transfer
probe-mode=happy
result=success
transfer-started=PASS
legacy-gds-file-storage=ABSENT
```

Matched hosted artifact:

| Source | Received | Bytes | SHA-256 |
|---|---|---:|---|
| `/tmp/comm-reliable-transfer-runtime/data-products/Dp_268693505_1779740867_00880582.fdp` | `/tmp/comm-reliable-transfer-hosted.hIcCeL/rt-output/Dp_268693505_1779740867_00880582.fdp` | `469` | `92058afd3900168da9f267b66cf0a27dfc952a23de1cf2ade96432f28405750e` |

## Hosted Degraded Resend Verdict

Verdict: `PASS` for one bounded ACK-loss/no-progress case that forces resend
before final success.

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-26` |
| command | `PROBE_MODE=ack-loss bash scripts/run_comm_reliable_transfer_hosted_probe.sh` |
| formal verdict | `reliable-transfer` |
| probe mode | `ack-loss` |
| result | `success` |
| COMM node | `5` |
| RT output dir | `/tmp/comm-reliable-transfer-hosted.YuNBof/rt-output` |
| GDS file storage | `/tmp/comm-reliable-transfer-hosted.YuNBof/gds-downlink` |
| logs | `/tmp/comm-reliable-transfer-hosted.YuNBof` |

Observed PASS markers:

```text
comm-reliable-transfer-hosted-probe: PASS
formal-verdict=reliable-transfer
probe-mode=ack-loss
result=success
resend-observed=PASS
legacy-gds-file-storage=ABSENT
```

Hosted OBC log fragments:

```text
COMM_RT_TRANSFER_STARTED : Comm reliable transfer 1 started bytes 469 segments 3
COMM_RT_RESEND : Comm reliable transfer 1 resend 1 contiguousSegments 0
COMM_RT_PROGRESS : Comm reliable transfer 1 progress segments 2/3 bytes 320 duplicates 2
COMM_RT_PROGRESS : Comm reliable transfer 1 progress segments 3/3 bytes 469 duplicates 2
COMM_RT_FINAL_RESULT : Comm reliable transfer 1 final result 1 bytes 469 duplicates 2
```

## Hosted Bounded Failure Verdict

Verdict: `PASS` for bounded retry exhaustion with no final artifact promotion.

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-26` |
| command | `PROBE_MODE=retry-exhausted bash scripts/run_comm_reliable_transfer_hosted_probe.sh` |
| formal verdict | `reliable-transfer` |
| probe mode | `retry-exhausted` |
| result | `retry-exhausted` |
| COMM node | `5` |
| RT output dir | `/tmp/comm-reliable-transfer-hosted.wh4kPf/rt-output` |
| GDS file storage | `/tmp/comm-reliable-transfer-hosted.wh4kPf/gds-downlink` |
| logs | `/tmp/comm-reliable-transfer-hosted.wh4kPf` |

Observed PASS markers:

```text
comm-reliable-transfer-hosted-probe: PASS
formal-verdict=reliable-transfer
probe-mode=retry-exhausted
result=retry-exhausted
retry-exhausted-event=PASS
rt-output-final-file=ABSENT
legacy-gds-file-storage=ABSENT
```

Hosted OBC log fragments:

```text
COMM_RT_TRANSFER_STARTED : Comm reliable transfer 1 started bytes 469 segments 3
COMM_RT_RESEND : Comm reliable transfer 1 resend 1 contiguousSegments 0
COMM_RT_RESEND : Comm reliable transfer 1 resend 2 contiguousSegments 0
COMM_RT_RETRY_EXHAUSTED : Comm reliable transfer 1 retry exhausted resendCount 3 contiguousSegments 0 bytes 0
COMM_RT_FINAL_RESULT : Comm reliable transfer 1 final result 3 bytes 0 duplicates 2
```

## Target/Lab Happy-Path Verdict

Verdict: `PASS` for the target/lab default node-`5` happy-path bounded
reliable-transfer proof on the governed service-managed SocketCAN baseline.

Final passing run:

| Field | Value |
|---|---|
| date | `2026-05-26` |
| command | `bash scripts/run_comm_csp_socketcan_reliable_transfer_probe.sh` |
| formal verdict | `reliable-transfer` |
| mode | `reliable-transfer` |
| profile | `sband` |
| probe root | `/tmp/comm-csp-socketcan-reliable-transfer.YGdQD6` |
| session-open source | `target-journal` |
| source path | `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc/data-products/Dp_268693505_1779774403_00483451.fdp` |
| received path | `/tmp/comm-reliable-transfer-node5/Dp_268693505_1779774403_00483451.fdp` |
| received size | `469` |
| SHA-256 | `1e397e620021e4d5759c8a572eb775f2e2e88745af288397be60d5d9467271e9` |
| RT output dir | `/tmp/comm-reliable-transfer-node5` |
| GDS file storage | `absent` |

Observed PASS markers:

```text
target-can-matrix-probe: PASS
mode=reliable-transfer
profile=sband
source-path=$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc/data-products/Dp_268693505_1779774403_00483451.fdp
received-path=/tmp/comm-reliable-transfer-node5/Dp_268693505_1779774403_00483451.fdp
legacy-gds-file-storage=absent
```

Target/lab proof notes:

- the repository-owned wrapper resets target runtime `data-products` to a
  bounded fresh official `.fdp` set before `DpCatalog.BUILD_CATALOG`, so the
  proof does not inherit unbounded historical pending catalog state
- the proof still reuses the current default node-`5` operational path already
  registered in entry `59`; this evidence adds a reliable-transfer capability
  claim on that governed path, but does not create a new reusable path entry

## Verification

Closeout checks completed in this session:

| Step | Command | Result |
|---|---|---|
| syntax check | `bash -n scripts/run_comm_reliable_transfer_hosted_probe.sh` | PASS |
| syntax check | `bash -n scripts/run_comm_csp_socketcan_reliable_transfer_probe.sh` | PASS |
| syntax check | `python3 -m py_compile scripts/comm_verification/lib/run_target_can_matrix_probe.py` | PASS |
| syntax check | `python3 -m py_compile scripts/comm_verification/lib/run_target_tcp_matrix_probe.py` | PASS |
| fresh local unit/integration focus | `ctest --test-dir build-fprime-automatic-native-ut -R '^(OBC_Components_CommController_ut_exe|comm_reliable_transfer_protocol_unit_test)$' --output-on-failure` | PASS |
| hosted happy-path proof | `bash scripts/run_comm_reliable_transfer_hosted_probe.sh` | PASS |
| hosted degraded resend proof | `PROBE_MODE=ack-loss bash scripts/run_comm_reliable_transfer_hosted_probe.sh` | PASS |
| hosted bounded failure proof | `PROBE_MODE=retry-exhausted bash scripts/run_comm_reliable_transfer_hosted_probe.sh` | PASS |
| target/lab happy-path proof | `bash scripts/run_comm_csp_socketcan_reliable_transfer_probe.sh` | PASS |
