# CubeSat OBC 飛行軟體

[English](README.md) ·
[系統架構](docs/architecture.md) ·
[介面契約](docs/interfaces.md) ·
[驗證總覽](docs/verification.md) ·
[論文技術索引](docs/thesis.md) ·
[展示流程](docs/operator/thesis-demo.zh-TW.md)

本專案是一套基於 F Prime v4.1.0 的 CubeSat 星載電腦軟體原型，整合任務
模式、自主序列、酬載操作、子系統通訊、故障復原、安全指令、資料產品、
Raspberry Pi 部署與地面操作介面。

核心成果包括：

- 以 `OBC/TopCcsds/topology.fpp` 定義單一 OBC deployment；
- 透過 internal CSP 整合 EPS、ADCS、COMM、GPS 與 payload；
- 支援 CCSDS S-band 及 UHF primary/failover 通訊路徑；
- 以 challenge-response、session sequence 與 authority gate 保護指令；
- 提供 FDIR、子系統 recovery、process restart 及 hardware watchdog；
- 使用 F Prime `.fdp` 建立任務歷史資料產品與檔案下傳；
- 提供 Mission Console、F Prime GDS、模擬器及 Chapter 5 展示路線；
- 以 OpenSpec 保存需求、設計、決策與可追溯的開發歷史。

## 快速建置

```bash
python3 -m venv fprime-venv
fprime-venv/bin/pip install -r requirements.txt
bash scripts/bootstrap_dev_config.sh
fprime-venv/bin/fprime-util generate -f
fprime-venv/bin/fprime-util build
```

啟動 hosted 環境：

```bash
bash scripts/run_dev_stack.sh
```

執行完整驗證：

```bash
bash scripts/run_verification_ci.sh
```

公開的 command-auth 設定只用於本機模擬與 CI；target package 必須透過
`OBC_PACKAGE_KEYSTORE_PATH` 提供私有 keystore。授權與第三方來源請見
[LICENSE](LICENSE) 及 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
