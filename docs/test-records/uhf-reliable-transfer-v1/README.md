# UHF Reliable Transfer V1

Status: final branch evidence for formal sync/archive.
Last updated: 2026-05-30.

## Scope Under Test

- Exact artifact family: current official HK `.fdp` whole-file requests only
- Exact reliable-transfer paths:
  - default S-band node `5`
  - explicit-switched `uhf-primary-after-failover` node `6`
- Exact target/lab proof boundary:
  - node-`5` bootstrap
  - explicit `COMM_SET_ACTIVE(UHF)`
  - quiet switched node-`6` happy-path reliable transfer only
- Exact non-claims:
  - no `uhf-backup` reliable transfer
  - no automatic failover-to-UHF reliable transfer
  - no nominal non-quiet target UHF reliable-transfer promotion
  - no RF, restart-persistent resume, broad CFDP, one-GDS aggregation,
    one-gateway multiplexing, or generic simultaneous closure

## Reused From `reliable-transfer-v1`

- `CommController` remains policy/admission owner
- `DpCatalog` remains file-selection owner
- `ground_ttc_gateway` remains raw relay only
- helper transport semantics remain:
  - `160`-byte RT `DATA` payload
  - window `2`
  - ACK timeout `1`
  - resend budget `3`
  - `64` segments / `10,240` bytes
  - one active transfer
  - duplicate-in-context ignore
  - no restart-persistent resume

## Path-Local Budget Facts

- Stock current UHF official file/downlink ceiling remains `243` file-data
  bytes per `Fw::FilePacket::DATA` packet.
- The bounded reliable helper still uses `160`-byte RT `DATA` payloads.
- The bounded reliable helper still caps one local transfer at `64` segments /
  `10,240` bytes.

## Fresh Local Verification

PASS:

- `bash -n scripts/run_comm_uhf_reliable_transfer_hosted_probe.sh`
- `bash -n scripts/run_comm_csp_socketcan_uhf_reliable_transfer_probe.sh`
- `python3 -m py_compile scripts/comm_verification/lib/run_hosted_uhf_reliable_transfer_probe.py scripts/comm_verification/lib/run_target_can_matrix_probe.py`
- `PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native-ut --target OBC_Components_CommController_ut_exe`
- `PATH="$PWD/fprime-venv/bin:$PATH" ctest --test-dir build-fprime-automatic-native-ut -R '^OBC_Components_CommController_ut_exe$' --output-on-failure`
- `PATH="$PWD/fprime-venv/bin:$PATH" ctest --test-dir build-fprime-automatic-native-ut -R '^comm_reliable_transfer_protocol_unit_test$' --output-on-failure`
- `PATH="$PWD/fprime-venv/bin:$PATH" cmake --build build-fprime-automatic-native --target OBC`

## Hosted Proof

The hosted wrapper uses the maintained per-band stock-stack launcher rather
than an ad hoc stack bring-up. The proof therefore reuses launcher-owned
runtime roots, coordinated cleanup, and stock S-band/UHF ground surfaces on
one shared hosted `TopCcsds` runtime.

The authoritative hosted evidence below was rerun with unrestricted execution
after clearing stale local probe helpers and repo-root `.adm-*` / `.stg-*`
aliases. After each hosted rerun, local process inspection stayed clear of
orphaned `fprime-gds`, `fprime-cli`, `ground_ttc_gateway`, `csp_zmqproxy`, and
COMM-node leftovers, and no new repo-root sequence aliases reappeared.

| Case | Command | Result | Artifact root |
|---|---|---|---|
| happy path | `bash scripts/run_comm_uhf_reliable_transfer_hosted_probe.sh` | `PASS` | `/tmp/comm-uhf-reliable-transfer-hosted.j7u9pP` |
| resend-before-success | `PROBE_MODE=ack-loss bash scripts/run_comm_uhf_reliable_transfer_hosted_probe.sh` | `PASS` with resend observed | `/tmp/comm-uhf-reliable-transfer-hosted.wK0aq0` |
| retry exhausted | `PROBE_MODE=retry-exhausted bash scripts/run_comm_uhf_reliable_transfer_hosted_probe.sh` | `PASS` with no final artifact promotion | `/tmp/comm-uhf-reliable-transfer-hosted.9vBimG` |

Hosted evidence facts:

- all three cases use explicit switch to `uhf-primary-after-failover`
- the repo-owned node-`6` RT output is the only success surface
- stock GDS file storage remains absent on the reliable-transfer path
- happy path reports `formal-verdict=uhf-reliable-transfer`,
  `comm-node=6`, and `uhf-path=uhf-primary-after-failover`
- retry-exhausted reports `rt-output-final-file=ABSENT` and
  `legacy-gds-file-storage=ABSENT`

## Target/Lab Proof

Preparation and deployment:

- `bash scripts/package_rpi_bundle.sh`
- `bash scripts/install_rpi_bundle.sh`

Clean-baseline preflight:

- after a full transport baseline reset, `TARGET_COMM_PROFILE=sband PROBE_MODE=command-path bash scripts/run_rpi_target_recovery_restart_probe.sh`
  passed on the same governed lab baseline
- comparator artifact root: `/tmp/rpi-target-recovery-restart.7fRd8k`

Governed proof command:

- `bash scripts/run_comm_csp_socketcan_uhf_reliable_transfer_probe.sh`

Result:

- `PASS`
- artifact root: `/tmp/comm-csp-socketcan-uhf-reliable-transfer.k26f1R`

Target/lab path facts:

- default node-`5` `sband-primary` session opened first
- explicit `COMM_SET_ACTIVE(UHF)` switch succeeded
- quiet node-`6` `uhf-primary-after-failover` session opened on the governed
  UHF path
- `BUILD_CATALOG` plus `START_XMIT_CATALOG(NO_WAIT)` selected one official HK
  `.fdp` source file
- target journal recorded:
  - `COMM_RT_ROUTE_SELECTED`
  - `COMM_RT_TRANSFER_STARTED`
  - bounded resend and progress events
  - `COMM_RT_FINAL_RESULT`
- node-`6` receiver wrote the byte-matching artifact:
  - source: `$OBC_HOME/obc-deploy/runtime/comm-csp-lab-obc/data-products/Dp_268693505_1780084945_00941915.fdp`
  - received: `/tmp/comm-reliable-transfer-node6/Dp_268693505_1780084945_00941915.fdp`
  - size: `469`
- stock GDS file storage remained absent for the reliable-transfer path

Cleanup and restore-to-normal evidence:

- checkpoint `reliable-transfer-receiver-override-removed`: `pass`
- checkpoint `reliable-transfer-admission-override-removed`: `pass`
- checkpoint `quiet-override-removed`: `pass`, `source=service-environment`
- checkpoint `reliable-transfer-nonquiet-baseline-restored`: `pass`
- post-proof service environments returned to:
  - `subsystem-uhf-csp.service` without `DIAGNOSTIC_QUIET_PACKET_EGRESS`
  - `obc-comm-csp-stack.service` with `DIAGNOSTIC_QUIET_PACKET_EGRESS=0`

## What Was Newly Proven For UHF

- `CommController` admits the bounded reliable helper on exact
  explicit-switched `uhf-primary-after-failover` node `6`
- helper target-node selection is explicit for node `5` versus node `6`
- in-flight transfer contexts abort/fail boundedly on switch or failover and
  do not migrate across paths
- repository-owned hosted proof now closes happy-path, resend-before-success,
  and retry-exhausted for the switched UHF node-`6` slice
- repository-owned target/lab proof now closes the exact quiet switched
  node-`6` happy path and records proof-owned quiet cleanup plus restoration to
  the governed normal non-quiet baseline

## Residual Non-Claims

- no `uhf-backup` reliable transfer
- no automatic failover-to-UHF reliable transfer
- no nominal non-quiet target UHF reliable-transfer promotion
- no RF or over-the-air closure
- no restart-persistent resume
- no broad CFDP
- no one-GDS aggregation
- no one-gateway multiplexer
- no generic simultaneous closure

## Verdict

- Hosted switched-UHF happy path: `PASS`
- Hosted switched-UHF resend-before-success: `PASS`
- Hosted switched-UHF retry exhausted: `PASS`
- Target/lab quiet switched node-`6` happy path with restore-to-normal cleanup:
  `PASS`
- This change closes the remaining practical COMM capability gap for the
  repository's current no-RF simulation scope while keeping all broader
  non-claims explicit.
