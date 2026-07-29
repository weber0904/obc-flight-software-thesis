# CubeSat OBC 飛行軟體論文公開版

本 repo 是 `thesis-submission-v1` 的公開策展版本，核心基於 F Prime
v4.1.0，保留目前可建置的 OBC 實作、完整 OpenSpec、測試紀錄摘要及現行
驗證入口；過時腳本、論文內文草稿、交接筆記與重複報告已從公開操作面移除。

主要內容：

- `OBC/TopCcsds/topology.fpp` 是唯一 maintained deployment。
- 以 internal CSP 串接 EPS、ADCS、COMM 與 payload。
- 具備 CCSDS S-band、受治理的 UHF primary/failover、secure command、
  FDIR/recovery、boot trust、official F Prime data products 和 Mission
  Console。
- OpenSpec 保留正式需求、設計決策與變更歷史。
- 大型 raw evidence 放在附 SHA-256 的 GitHub Release asset；Git 內保留
  claim、verdict、環境、限制及索引。

本版沒有在最終公開 commit 重新執行 Raspberry Pi／實驗室硬體測試。
相關結果只標示為「先前已展示」，並保留當時 commit、日期、環境及差異，
不宣稱是 `thesis-submission-v1` 的 fresh target proof。

閱讀順序：

1. [英文主 README](README.md)
2. [現行架構](docs/architecture/current-development-architecture.md)
3. [本專案貢獻](docs/architecture/project-contributions.md)
4. [驗證矩陣](docs/verification-matrix.md)
5. [論文 claim／evidence 對照](docs/thesis/claim-evidence-map.zh-TW.md)
6. [Route 1/2/3 展示流程](docs/operator/thesis-demo-routes.zh-TW.md)

本專案是研究原型，不是 flight-certified 軟體。公開 example keys 只能供
hosted demo／CI 使用，不可部署到 target。
