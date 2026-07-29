# Thesis Submission Route 1／2／3 展示手冊

狀態：目前有效的繁中論文展示入口。  
更新日期：2026-07-29。

一般 hosted 操作與 Mission Console 以
[`mission-console-phase1-runbook.md`](mission-console-phase1-runbook.md) 為準。
本文件只定義論文展示順序與 oracle。

## 共用準備

```bash
bash scripts/bootstrap_dev_config.sh
bash scripts/run_verification_ci.sh
```

啟動 hosted manual surface、Mission Console 及對應 CLI listeners時，使用獨立
runtime root、file store及 ports。公開 example keys 只適用 hosted demo。

## Route 1：Sequence-Driven Payload

```bash
bash scripts/chapter5_routes/hosted/run_route1_hosted.sh
```

展示：

- governed sequence admission與執行
- payload capture acknowledgement
- preview/raw official data products
- downlink與decode provenance

不宣稱：公開 tag 上的 real-camera target proof。

## Route 2：Mode、TTC 與 Link Recovery

```bash
bash scripts/chapter5_routes/hosted/run_route2_hosted.sh
```

展示：

- SAFE／IDLE／TTC mode policy
- TTC window條件
- S-band unavailable window
- autonomous UHF policy與HK file continuity

Hosted route不等於physical UHF或RF proof。

## Route 3：FDIR 與 Recovery

```bash
bash scripts/chapter5_routes/hosted/run_route3_hosted.sh
```

展示 ADCS reset、EPS recovery/safe fallback與reboot前恢復鏈。Raspberry Pi
hardware-watchdog board reset只引用原始 target test record，不在本 tag 重跑。

## Target Evidence 閱讀規則

Target腳本仍保留供未來重跑，但本 release只接受：

- test record所列原始 commit/date
- 當時target、subsystem與ground拓樸
- manifest、service、dictionary及keystore provenance
- `previously demonstrated` 標示

任何 packaging、credential或oracle差異都必須留在release delta，不可寫成
`verified on thesis-submission-v1 target`。
