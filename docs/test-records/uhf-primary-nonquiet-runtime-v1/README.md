# 測試紀錄：uhf-primary-nonquiet-runtime-v1

## Scope

- 日期：2026-06-25
- 分支：`feature/chapter5-integrated-route-closure-v1`
- 關聯 change：
  `openspec/changes/archive/2026-06-24-uhf-primary-nonquiet-autofailover-v1/`
- 目標：以 current resend-ground-readback policy 驗證 maintained non-quiet
  `UHF primary` command/readback runtime

Durable navigation:

- current autonomous failover proof：
  [target-autonomous-uhf-failover-v1](../target-autonomous-uhf-failover-v1/README.md)
- Chapter 5 Route 2/3 current closure：
  [chapter5-integrated-route-closure-v1](../chapter5-integrated-route-closure-v1/README.md)

## Current Acceptance

這條 benchmark 的 current acceptance 仍是：

- fresh `A/B` baseline
- `UHF primary` non-quiet runtime
- 兩個子案例各 `10` 次
- 每個子案例 `>= 7/10` 才可往下引用於 Route 2/3 target closure

這輪 current readback oracle 已不是舊的 single-shot visibility，而是：

- secure command 送出後等待 ground fresh readback `1s`
- 若未看到對應 fresh readback 就重送
- 最多 `30` 次 total sends
- 預設 same-seq retry
- 若 target 已接受該 seq 或出現 duplicate/sequence reject，下一次改送 new seq

target journal 只作 retry decision 與 failure diagnosis，不再作成功 oracle。

## Fresh Governed Rerun

執行前先 fresh 跑：

```bash
bash scripts/ensure_target_comm_lab_baseline.sh
bash scripts/ensure_ground_dual_gds_baseline.sh
```

執行命令：

```bash
bash scripts/run_target_uhf_primary_nonquiet_runtime_probe.sh
```

artifact root：

- `/private/tmp/target-uhf-primary-nonquiet-runtime.J93YnP/`

formal artifacts：

- [summary.log](/private/tmp/target-uhf-primary-nonquiet-runtime.J93YnP/summary.log)
- [uhf-primary-nonquiet-runtime-summary.json](/private/tmp/target-uhf-primary-nonquiet-runtime.J93YnP/diagnostics/uhf-primary-nonquiet-runtime-summary.json)
- [checkpoints.jsonl](/private/tmp/target-uhf-primary-nonquiet-runtime.J93YnP/diagnostics/checkpoints.jsonl)

## Observed Result

- top-level verdict：`PASS`
- repeated single-command：
  - `GET_RESET_CAUSE = 10/10`
- interleaved dual-command：
  - `GET_RESET_CAUSE + GET_PERSISTENT_FAULT_HISTORY = 10/10`

這代表 current maintained non-quiet `UHF primary` path 在 resend-ground-readback
policy 下，已達到本 change 要求的 success-rate gate。

## What This Proves

- current product semantics 已不是 `UHF primary packet quiet`
- failover 後的 `UHF primary` ground path 可以承接 bounded secure-command
  readback，而不是只能靠 target journal 自證
- `GET_RESET_CAUSE` 與 `GET_PERSISTENT_FAULT_HISTORY` 兩類 bounded readback
  都能在 current ground path 上穩定完成
- current resend policy 足以抵抗先前 single-shot path 暴露出的
  observability miss，而不需要把 target journal 當成功 oracle

## What This Does Not Prove

- generic UHF reliable-transfer closure
- arbitrary long-lived high-rate live telemetry under `UHF primary`
- autonomous failover 本身；那是另一條 proof
- Route 2/3 target 全流程本身；這份 benchmark 只是它們的 current supporting evidence

## Historical Context

舊的 2026-06-24 rerun 仍保留 review value，但已不是 current maintained truth：

- `/private/tmp/target-uhf-primary-nonquiet-runtime.wGU28F/`
  - probe infrastructure bug：缺少 current opcode mapping
- `/private/tmp/target-uhf-primary-nonquiet-runtime.PrXOTK/`
  - single-shot ground visibility path only reached:
    - repeated `GET_RESET_CAUSE = 6/10`
    - interleaved `GET_RESET_CAUSE + GET_PERSISTENT_FAULT_HISTORY = 4/10`

這些 historical result 說明過去 blocker 曾經真實存在，但它們已被 current
resend-ground-readback family supersede。之後不需要再反覆重跑這個 `10 x 2`
benchmark 來重新確認同一件事，除非 COMM ground-readback policy 再次改動。

## Verdict

- current maintained verdict：`PASS`
- allowed current use：
  - `uhf-primary-nonquiet-autofailover-v1` supporting evidence
  - Chapter 5 Route 2 target supporting evidence
  - Chapter 5 Route 3 target supporting evidence
