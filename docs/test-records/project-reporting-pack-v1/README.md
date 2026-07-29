# 測試紀錄：project-reporting-pack-v1

- 日期：2026-04-09
- 層級：L1 / governance and reporting-package maintenance
- 環境：
  - host: macOS development machine
  - mode: checked-in reporting package and review-surface alignment
- 關聯變更：`openspec/changes/archive/2026-04-08-project-reporting-pack-v1/`
- 關聯文件：
  - `docs/reporting/README.md`
  - `docs/reporting/project-reporting-pack-v1/README.md`
  - `docs/reporting/project-reporting-pack-v1/architecture-diagrams.md`
  - `docs/reporting/project-reporting-pack-v1/capability-status-matrix.md`
  - `docs/reporting/project-reporting-pack-v1/demo-runbook.md`

## 1. 目標

本 change 的目標是把目前 repo 的真實完成狀態、系統架構、驗證分層與穩定 demo 路徑整理成一套教授/PM 可讀的 checked-in reporting package，而不是再新增 flight runtime 功能。

## 2. Repo Truth Sources

本次 reporting package 以以下 checked-in sources 為準：

- `README.md`
- `AGENTS.md`
- `docs/verification-matrix.md`
- `docs/baseline-reconciliation-matrix.md`
- `docs/verification-path-registry.md`
- `openspec/specs/*`
- `OBC/Top/topology.fpp`
- `OBC/Top/instances.fpp`
- relevant `docs/test-records/*`

## 3. 本次實作重點

- 新增 `project-reporting` capability
- 新增 `docs/reporting/` index 與 `project-reporting-pack-v1/` package
- 新增 professor-facing briefing、Mermaid diagrams、capability matrix、demo runbook
- 補齊此 package 的 formal evidence、reconciliation、以及 verification review surface

## 4. 驗證指令

### 4.1 Repo consistency

```bash
python3 scripts/check_repo_consistency.py
```

### 4.2 Shared baseline gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/project-reporting-pack-v1-refresh
```

### 4.3 OpenSpec validation

```bash
openspec validate project-reporting-pack-v1
openspec validate --specs
```

### 4.4 Focused hosted demo-path sanity rerun

This reporting refresh also reran the exact hosted demo path used by the professor runbook. The rerun deliberately reused already-registered paths rather than introducing a new validation claim:

1. hosted direct `OBC -> GDS` TCP adapter path
2. hosted `fprime-cli -> GDS` command/uplink path for housekeeping archive smoke

Headless GDS:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" \
fprime-gds -n -g none --no-zmq \
  --framing-selection fprime \
  --dictionary "$PWD/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json" \
  --ip-address 127.0.0.1 \
  --ip-port 50120 \
  --tts-port 50121 \
  --file-storage-directory /tmp/prof-demo-downlink \
  --log-to-stdout
```

Hosted OBC stack:

```bash
RUNTIME_ROOT=/tmp/professor-demo-runtime \
GDS_HOST=127.0.0.1 \
GDS_PORT=50120 \
bash scripts/run_gds_stack.sh
```

Ground-side archive commands:

```bash
PATH="$PWD/fprime-venv/bin:$PATH" \
fprime-cli command-send \
  --dictionary "$PWD/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json" \
  --no-zmq \
  --tts-port 50121 \
  OBCApp.housekeepingArchive.HK_CAPTURE_NOW
```

```bash
PATH="$PWD/fprime-venv/bin:$PATH" \
fprime-cli command-send \
  --dictionary "$PWD/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json" \
  --no-zmq \
  --tts-port 50121 \
  OBCApp.housekeepingArchive.HK_DOWNLINK_INDEX
```

Observed artifacts:

```bash
find /tmp/prof-demo-downlink -maxdepth 3 -type f | sort
```

## 5. 觀察結果

- repo 現在有一套 checked-in 報告包，可直接支援 10-15 分鐘教授/PM 更新與短 demo 準備
- 報告包中的 scope、diagram、capability matrix 與 demo path 都以既有 formal baseline 和 archived evidence 為準
- 圖表已改成 minimal clean 風格，ground path、external comm simulation、simulator-backed subsystem path 與 future work 被分開敘事，不再把多條路徑混成一張複雜流程圖
- demo 主敘事已改成 hosted GDS-connected baseline，不再把本地 REPL 當成唯一主軸
- 這次沒有把 hosted baseline 誤講成 final hardware integration，也沒有把 future work 折疊成已完成能力
- 此 change 沒有改 flight runtime 行為，因此 live demo path 的可信度來自既有 governed evidence 與本次 focused sanity rerun，而不是重新宣稱新的 runtime bring-up

### 5.1 Focused demo-path rerun outcomes

- headless `fprime-gds` 使用 `ip-port=50120`、`tts-port=50121`、`file-storage-directory=/tmp/prof-demo-downlink` 成功啟動
- hosted OBC stack 使用 `RUNTIME_ROOT=/tmp/professor-demo-runtime`、`GDS_HOST=127.0.0.1`、`GDS_PORT=50120` 成功連上 GDS
- OBC log 顯示 direct TCP ground path connected:
  - `Ground link target: 127.0.0.1:50120`
  - `Connected to 127.0.0.1:50120 as a tcp client`
- ground-side `fprime-cli` commands 成功打進 OBC：
  - `HK_CAPTURE_NOW`
  - `HK_DOWNLINK_INDEX`
- OBC log 顯示 archive actions 完成：
  - `HK_CAPTURE_RECORDED`
  - `HK_INDEX_DOWNLINK_QUEUED`
  - `fileDownlink FileSent`
- downlink directory 產出：
  - `/tmp/prof-demo-downlink/fprime-downlink/hk-index.csv`
- 本次 rerun 沒有把 `radio_mock_server` 當成 GDS path 的中間層；它仍只是 hosted stack 一起啟動的 parallel external comm simulation peer
- 本次 rerun 採 headless GDS；UI 仍維持 optional，不作為教授 demo 成功與否的必要條件

### 5.2 Wording guardrail

- 不得把 `radio_mock_server` 說成 GDS ground path 的 RF middle layer
- 不得把 direct `OBC -> GDS` path 說成最終 flight-like ground architecture
- 不得把本次 demo path 解讀成 Raspberry Pi、real radio、或 GPS live UART hardware 已驗證

## 6. 測試結果

- `python3 scripts/check_repo_consistency.py`: `PASS`
- `bash scripts/run_verification_ci.sh build-artifacts/project-reporting-pack-v1-refresh`: `PASS`
- `openspec validate project-reporting-pack-v1` (pre-archive): `PASS`
- `openspec validate --specs`: `PASS`
- focused headless GDS + hosted OBC + `fprime-cli` archive demo-path rerun: `PASS`
- `/tmp/prof-demo-downlink/fprime-downlink/hk-index.csv` produced during rerun: `PASS`

## 7. 驗收結論

- `PASS`：repo 現在有一套 checked-in 教授／PM reporting package，可用於 10-15 分鐘口頭報告與穩定 demo 準備
- `PASS`：`project-reporting` capability、verification evidence、baseline reconciliation、verification matrix 都已與 archived change 對齊
- `PASS`：這次整理維持了 repo-truth 敘事邊界，沒有把未完成硬體擴展路徑講成已驗證 baseline
- `PASS`：修正版 reporting package 已有對應的 hosted GDS demo-path sanity rerun，可直接支援教授現場演示準備
