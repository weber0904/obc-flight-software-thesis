# 論文章節展示流程

三條路線將系統能力組合成可重現的 Chapter 5 情境。每個 runner 會建立自己的
runtime root、配置、port、log 與結果目錄。

## 共用準備

```bash
bash scripts/bootstrap_dev_config.sh
fprime-venv/bin/fprime-util generate -f
fprime-venv/bin/fprime-util build
```

## Route 1：Sequence-Driven Payload

```bash
bash scripts/chapter5_routes/hosted/run_route1_hosted.sh
```

觀察重點：

- sequence 編譯、admission 與 execution；
- payload capture acknowledgement；
- preview 與 raw data product；
- file downlink、extract 與 decode provenance。

完成後查看 runner 輸出的 result summary、sequence log、payload metadata
及下載檔案。

## Route 2：Mode、TTC 與 Link Recovery

```bash
bash scripts/chapter5_routes/hosted/run_route2_hosted.sh
```

觀察重點：

- SAFE、IDLE 與 TTC mode transition；
- SoC 與 TTC window admission；
- S-band link state 變化；
- UHF failover policy；
- HK file continuity。

需要手動改變 EPS 或 ADCS 輸入時，使用
[Simulator 控制介面](simulator-controls.zh-TW.md)。

## Route 3：FDIR 與 Recovery

```bash
bash scripts/chapter5_routes/hosted/run_route3_hosted.sh
```

觀察重點：

- ADCS timeout detector 與 reset executor；
- EPS timeout、recovery escalation 與 safe fallback；
- persistent fault history；
- process restart 前後的 recovery state。

Target watchdog 與 Raspberry Pi service restart 的操作方式請見
[Target And Lab Operations](target-lab.md)，對應結果收錄於
[Evidence Library](../../evidence/README.md)。

## 展示順序

建議依序展示：

1. 先由 Mission Console 說明 system context、telemetry 與 link state。
2. 執行 Route 1，呈現 command → sequence → payload → file 的資料流。
3. 執行 Route 2，呈現 mode policy 與通訊恢復。
4. 執行 Route 3，呈現 detector → recovery executor 的 FDIR 路徑。
5. 以 `docs/verification.md` 與 evidence record 回到測試與可追溯性。

## 結果索引

三條 route 的整合結果：

- [Chapter 5 Integrated Route Closure](../../evidence/records/chapter5-integrated-route-closure-v1/README.md)
- [Route 1 Sequence Verification](../../evidence/records/route1-sequence-verification-v1/README.md)
- [Recovery Executors](../../evidence/records/recovery-executors-v1/README.md)
