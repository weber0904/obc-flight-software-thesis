# 論文 Claim／Evidence 對照

狀態：`thesis-submission-v1` 公開對照表。  
更新日期：2026-07-29。

| 論文主張 | 實作來源 | 證據邊界 |
|---|---|---|
| F Prime 元件化 OBC 架構 | `OBC/Components/`、`OBC/TopCcsds/` | native build、classic component UT、topology |
| EPS／ADCS／COMM／Payload internal CSP | bridges、simulators、CSP runtime owner | fresh hosted integration；target 為先前已展示 |
| S-band CCSDS 主要路徑 | gateway、node `5`、TopCcsds | fresh hosted；target 紀錄保留原 commit |
| UHF primary／自主 failover | node `6`、COMM policy、FDIR | 實作保留；硬體 failover 為先前已展示 |
| Secure command authority | secure authorizer、ingress authority、helpers | fresh hosted secure-auth/command proof |
| Mode、TTC 與 mission sequencing | mode safety、mission autonomy、official sequencer | hosted Route 1/2/3 |
| Payload preview/raw data products | payload controller、official `.fdp` | fresh hosted Route 1；target artifacts 為歷史證據 |
| FDIR、recovery、watchdog | detectors、executors、watchdog | hosted recovery；target reset 為先前已展示 |
| 可追溯開發流程 | OpenSpec、registry、test records | 35 main specs、archived changes、evidence catalog |

所有 target/lab 主張均需回到 test record 查看日期、commit、設備與 non-claim；
不得由本表推論為公開 tag 的 fresh hardware closure。

本次公開候選版本的 fresh build、74 項註冊測試、hosted 路線選擇與
non-claim，集中記錄於
[`public-thesis-submission-v1`](../test-records/public-thesis-submission-v1/README.md)。
