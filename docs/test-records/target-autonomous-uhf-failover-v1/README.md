# 測試紀錄：target-autonomous-uhf-failover-v1

## Scope

- 日期：2026-06-25
- 分支：`feature/chapter5-integrated-route-closure-v1`
- 關聯 change：
  `openspec/changes/archive/2026-06-24-uhf-primary-nonquiet-autofailover-v1/`
- 目標：fresh rerun `scripts/run_target_autonomous_uhf_failover_probe.sh`

Durable navigation:

- non-quiet `UHF primary` runtime benchmark：
  [uhf-primary-nonquiet-runtime-v1](../uhf-primary-nonquiet-runtime-v1/README.md)
- Chapter 5 Route 2/3 current closure：
  [chapter5-integrated-route-closure-v1](../chapter5-integrated-route-closure-v1/README.md)

Current-note:

- current formal Chapter 5 rerun now preserves the maintained Route `2`
  target raw artifacts, including the autonomous failover proof root reused by
  the wrapper, under:
  [chapter5 route2/target canonical root](../chapter5-integrated-route-closure-v1/ARTIFACTS.json)
- this record remains the dedicated failover explanation surface; current raw
  artifact lookup should start from that canonical root

## Intended Proof

這條 proof 驗證 current maintained autonomous failover chain：

- node-`5` `S-band secure-auth`
- bounded `GET_RESET_CAUSE` precheck
- governed `subsystem-sband-csp.service` unavailable window
- `COMM_PRIMARY_UNAVAILABLE`
- executor-owned failover to `UHF primary`
- stale primary-side auth invalidation
- `UHF secure-auth` re-bootstrap
- accepted `UHF` auth 後 `COMM_UHF_BEACON_SUPPRESS_STARTED`
- bounded `GET_RESET_CAUSE`
- bounded `GET_PERSISTENT_FAULT_HISTORY`

這輪 bounded `GET_*` readback 使用 current resend-ground-readback policy；
ground fresh readback 才是成功 oracle。

## Fresh Governed Rerun

執行前先 fresh 跑：

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
```

執行命令：

```bash
bash scripts/run_target_autonomous_uhf_failover_probe.sh
```

artifact root：

- `/private/tmp/target-autonomous-uhf-failover.wWrASs/`

formal artifacts：

- [summary.log](/private/tmp/target-autonomous-uhf-failover.wWrASs/summary.log)
- [autonomous-uhf-failover-summary.json](/private/tmp/target-autonomous-uhf-failover.wWrASs/diagnostics/autonomous-uhf-failover-summary.json)
- [checkpoints.jsonl](/private/tmp/target-autonomous-uhf-failover.wWrASs/diagnostics/checkpoints.jsonl)

## Observed Result

- top-level verdict：`PASS`
- `S-band secure-auth bootstrap`：`PASS`
- bounded `GET_RESET_CAUSE` precheck：`PASS`
- governed unavailable window：`PASS`
- autonomous failover markers retained：
  - `COMM_PRIMARY_UNAVAILABLE`
  - `COMM_PRIMARY_LINK_CHANGED`
  - `COMM_RECOVERY_FAILOVER_RESULT switched True`
- `UHF secure-auth` re-bootstrap：`PASS`
- `COMM_UHF_BEACON_SUPPRESS_STARTED`：`PASS`
- bounded `GET_RESET_CAUSE` on `UHF primary`：`PASS`
- bounded `GET_PERSISTENT_FAULT_HISTORY` on `UHF primary`：`PASS`

## What This Proves

- current failover owner 是 detector + `RecoveryExecutor`，不是 manual
  `COMM_SET_ACTIVE(UHF)` operator step
- failover 後舊 primary-side auth 不能直接沿用，`UHF` 需要重新 secure-auth
- `UHF beacon suppress` 仍然是 auth-triggered 語意，不是 promotion-immediate
- failover 後的 current non-quiet `UHF primary` path 已能完成 bounded ground
  readback，不必靠 quiet mode 或 target-journal-only oracle 收尾

## What This Does Not Prove

- automatic restore back to nominal `S-band primary`
- generic `UHF` reliable-transfer or broad file authority
- Route 2 TTC / ADCS behavior
- Route 3 watchdog reboot behavior

## Historical Context

舊的 2026-06-24 attempts 仍保留 review value，但已被這次 fresh PASS supersede：

- `/private/tmp/target-autonomous-uhf-failover.vLssh4/`
  - proof oracle bug：auth refresh 後錯把 `STARTED` 當成仍必須出現的 marker
- `/private/tmp/target-autonomous-uhf-failover.fVN7bZ/`
  - one-shot readback path still failed on bounded `GET_RESET_CAUSE`

這些 historical failures 說明：

- earlier blocker 真實存在
- beacon oracle 曾有 probe bug
- single-shot readback family 不足以當 current maintained truth

current maintained truth 已改為 resend-ground-readback family，且本 record 的
fresh rerun 已經把這條 proof 關掉。

## Verdict

- current maintained verdict：`PASS`
- allowed current use：
  - `uhf-primary-nonquiet-autofailover-v1` closure evidence
  - Chapter 5 Route 2 target current closure dependency
  - Chapter 5 Route 3 target current closure dependency
