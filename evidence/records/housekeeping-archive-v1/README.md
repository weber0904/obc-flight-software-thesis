# 測試紀錄：housekeeping-archive-v1

- 日期：2026-04-04
- 層級：L2 / L3
- 環境：
  - host: macOS development machine
  - target-rpi: Raspberry Pi 64-bit Linux, isolated workspace `$OBC_HOME/lab/fprime/v0-hk-verify`
  - mode: hosted OBC runtime build + component integration validation + Raspberry Pi native generate/build/smoke
- 關聯變更：`openspec/changes/housekeeping-archive-v1/`
- 關聯文件：
  - `openspec/changes/housekeeping-archive-v1/specs/housekeeping-archive/spec.md`
  - `openspec/changes/housekeeping-archive-v1/specs/resource-storage/spec.md`
  - `openspec/changes/housekeeping-archive-v1/specs/verification-evidence/spec.md`
  - `evidence/verification-path-registry.md`

> Historical note (2026-05-27): any `operator@youjun.local` reference below is historical evidence only.
> Current governed OBC target references are `operator@obc.local` or `operator@<private-lab-host>`.

## 1. 目標

本 change 的目標是在既有 hosted OBC baseline 上補齊第一版 housekeeping archive：

- 不新增第二條 subsystem polling path
- 持續把 cached/runtime state 寫進 bounded ring archive
- 用 `slot + generation` 識別每一份 archive 檔案
- 生成可下載的 `index.csv`
- 透過既有 `FileDownlink` 路徑下傳 index 或指定 slot

本次仍然**不**處理：

- ground-pass 自動觸發 downlink
- by-time-range 自動選檔
- `DpWriter` / `DpCatalog` catalog workflow

## 2. 本次實作重點

- 新增 `OBC/Components/HousekeepingArchive/`
  - `HousekeepingArchive` passive component
  - `HousekeepingArchiveStore`，負責 ring files、rotation、generation 與 `index.csv`
  - repository-owned binary record format
- 新增 `OBC/Top/HousekeepingSnapshotProvider.*`
  - 從既有 runtime accessor 收集 snapshot
  - 來源包含 `ModeManager`、`EpsBridge`、`AdcsBridge`、`CommController`、`CspBridge`、`RadioController`、`UartDriver`、`BootManager`
- topology 新增 `housekeepingArchive`
  - 接入 slow rate group
  - 接入既有 `FileHandling.fileDownlink`
- `RadioController` 新增 cached status runtime accessor，避免 HK archive 為了 radio state 額外打 transport poll

## 3. 驗證指令

### 3.1 重新產生 UT build

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/fprime-venv/bin/fprime-util generate --ut -f
```

### 3.2 建置 UT tree

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/fprime-venv/bin/fprime-util build --ut
```

### 3.3 直接執行新的 housekeeping archive integration test

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/housekeeping_archive_integration_test
```

### 3.4 驗證受影響的 RadioController UT

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_RadioController_ut_exe
```

### 3.5 跑完整本機 regression

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/fprime-venv/bin/fprime-util check --all
```

### 3.6 hosted live command/downlink smoke

本段驗的是 **hosted `fprime-cli -> GDS` internal command listener -> OBC** 這條 command/uplink path，並沿用既有 hosted `OBC -> GDS` TCP adapter baseline；它不是重新定義或取代 `gds-ground-integration-v1` 的 direct TCP evidence。

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
fprime-gds -n -g none --no-zmq --framing-selection fprime \
  --dictionary $REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json \
  --ip-address 127.0.0.1 --ip-port 50102 --tts-port 50052 \
  --file-storage-directory /tmp/hk-smoke --log-to-stdout
```

```bash
RUNTIME_ROOT=/tmp/hk-runtime-hosted GDS_HOST=127.0.0.1 GDS_PORT=50102 \
  bash scripts/run_dev_stack.sh
```

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
fprime-cli command-send \
  --dictionary $REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json \
  --no-zmq --tts-port 50052 OBCApp.housekeepingArchive.HK_CAPTURE_NOW
```

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
fprime-cli command-send \
  --dictionary $REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json \
  --no-zmq --tts-port 50052 OBCApp.housekeepingArchive.HK_DOWNLINK_INDEX
```

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
fprime-cli command-send \
  --dictionary $REPO_ROOT/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json \
  --no-zmq --tts-port 50052 OBCApp.housekeepingArchive.HK_DOWNLINK_SLOT --arguments 0 0
```

### 3.7 Raspberry Pi native generate/build + runtime smoke

```bash
RPI_REMOTE_DIR=$OBC_HOME/lab/fprime/v0-hk-verify bash scripts/sync_rpi_workspace.sh
```

```bash
RPI_REMOTE_DIR=$OBC_HOME/lab/fprime/v0-hk-verify SKIP_SYNC=1 \
  bash scripts/bootstrap_rpi_workspace.sh
```

```bash
ssh operator@youjun.local '
  cd $OBC_HOME/lab/fprime/v0-hk-verify &&
  export PATH=$OBC_HOME/lab/fprime/v0-hk-verify/fprime-venv/bin:$PATH &&
  fprime-util generate -f &&
  fprime-util build
'
```

```bash
ssh operator@youjun.local '
  cd $OBC_HOME/lab/fprime/v0-hk-verify &&
  export PATH=$OBC_HOME/lab/fprime/v0-hk-verify/fprime-venv/bin:$PATH &&
  RUNTIME_ROOT=$OBC_HOME/lab/fprime/v0-hk-verify/runtime/integ-rpi \
  GDS_PORT=0 timeout 15 bash scripts/run_dev_stack.sh
'
```

## 4. 觀察結果

### 4.1 Archive storage model

- archive root：`<runtime-root>/hk/`
- index path：`<runtime-root>/hk/index.csv`
- slot file naming：`hk-00.bin`、`hk-01.bin`、...
- operator downlink naming：
  - index：`hk-index.csv`
  - slot：`hk-slot-<slot>-g<generation>.bin`

### 4.2 Rotation and generation behavior

- v1 archive 使用 fixed-count ring slots
- 當下一筆 record 會超過單檔大小限制時，active slot 會 rotate 到下一個 slot
- 若 slot 被 wraparound reuse，`generation` 會遞增
- reboot/restart 後不 resume-in-place，而是以前一個最近寫入 slot 為基準，繼續往下一個 slot 開新 generation
- 預設單檔大小限制是 `4096 bytes`
- 依目前 `HousekeepingSnapshot` record layout，單筆 serialized record 約 `216 bytes`
- hosted live evidence 觀察到 full slot file 會在 `18` 筆 record 時停在 `3908 bytes`，下一筆才 rotate

### 4.3 Index coverage

- `index.csv` 會記錄：
  - `slot`
  - `generation`
  - `active`
  - `record_count`
  - `file_size_bytes`
  - `start_time_*`
  - `end_time_*`
  - `file_name`
- stale `(slot, generation)` request 會被 reject，不會錯送另一份檔案

### 4.4 測試結果

- `housekeeping_archive_integration_test`: `PASS`
  - periodic capture 導致 slot rotation
  - wraparound reuse 會提升 generation
  - `index.csv` 內容與 active/inactive slot 狀態一致
  - stale generation request 會被拒絕
  - restart 後會以前一個最近寫入 slot 為基準繼續輪替
- `OBC_Components_RadioController_ut_exe`: `PASS`
  - 4/4 tests passed
- `fprime-util check --all`: `PASS`
  - 16/16 tests passed
  - 包含既有 simulator integration tests、既有 OBC component UT、mission autonomy integration tests，以及新的 housekeeping archive integration test
- hosted live command/downlink smoke: `PASS`
  - `HK_CAPTURE_NOW` 透過 `fprime-cli -> GDS -> OBC` 被實際 dispatch，hosted runtime 觀察到 `HK_CAPTURE_RECORDED`
  - `HK_DOWNLINK_INDEX` 產生 `/tmp/hk-smoke/fprime-downlink/hk-index.csv`
  - `HK_DOWNLINK_SLOT 0 0` 產生 `/tmp/hk-smoke/fprime-downlink/hk-slot-00-g000000.bin`
  - downlinked slot file 曾以 `cmp -s` 驗證，與 source `/tmp/hk-runtime-hosted/hk/hk-00.bin` byte-for-byte 一致
  - 本段新增證明的是 hosted `fprime-cli/TTS` command path，不是重新證明 direct `OBC -> GDS` TCP adapter path
  - rapid repeated `HK_CAPTURE_NOW` 會撞到 `COM_QUEUE` overflow，因此 live command burst 不是 rotation 驗收標準；rotation 仍以 hosted archive evidence 與 integration test 為主
- runtime-root launcher propagation: `PASS`
  - supplemental verification 發現 `run_dev_stack.sh`、`run_uart_stack.sh`、`packaging/rpi/launch/run_stack.sh`、`run_rpi_boot_probe.sh` 原先沒有把 `RUNTIME_ROOT` 傳成 `--runtime-root`
  - 修正後，custom runtime root 不再只影響 `persistent/staging`，也會一致地影響 `HousekeepingArchive` source path
- Raspberry Pi native generate/build: `PASS`
  - `fprime-util generate -f` 與 `fprime-util build` 在 `$OBC_HOME/lab/fprime/v0-hk-verify` 完成
  - 產生 target binary `$OBC_HOME/lab/fprime/v0-hk-verify/build-artifacts/Linux/OBC/bin/OBC`
- Raspberry Pi runtime smoke: `PASS`
  - `timeout 15 bash scripts/run_dev_stack.sh` 在 target 上成功啟動
  - target runtime 觀察到 `HK_CAPTURE_RECORDED`
  - `$OBC_HOME/lab/fprime/v0-hk-verify/runtime/integ-rpi/hk/` 下可見 `hk-00.bin` 與 `index.csv`
- Raspberry Pi bootstrap recovery note
  - 補驗證期間曾觀察到 `v0-hk-verify/fprime-venv/bin/fprime-util` 變成 `0 bytes`
  - 原因與前一輪中途斷電留下的半套 venv 一致；重新把 venv 套件拉回 repo pinned `lib/fprime/requirements.txt` 後恢復正常

## 5. 驗收結論

- `PASS`：第一版 `HousekeepingArchive` 已建立，且可從既有 cached/runtime state 持續產生 ring archive
- `PASS`：archive rotation、generation 與 index generation 已有 hosted evidence
- `PASS`：governed index/slot downlink request 已能重用既有 `FileDownlink` path
- `PASS`：stack launcher 現在會正確把 `runtime-root` 傳到底層 OBC，避免 HK archive source path 漂回預設值
- `PASS`：Raspberry Pi target 已完成 native generate/build 與 runtime smoke，確認 HK archive 至少可在 target filesystem 上生成
- `PASS`：本 change 未破壞既有 simulator、autonomy、comm、或 OBC component regression baseline

## 6. 邊界與限制

- 目前 archive cadence 與 slot/file-size policy 仍是 repository-owned fixed defaults
- 目前地面端仍需先看 `index.csv` 再決定要抓哪些 slot，尚未做 time-range 自動選檔
- 目前 archive record format 是 repository-owned binary format，不是 `DpWriter` / `DpCatalog` immutable data-product model
- hosted live GDS/TTS path 對 rapid repeated command injection 仍可能出現 queue pressure；本 change 不把 burst-rate tuning 當主要驗收面
- Raspberry Pi target 目前只做到 native generate/build 與 local runtime smoke，尚未在 target 上補 `fprime-cli/GDS` live downlink smoke
- `Blocked-HW`：本次仍未覆蓋真實 radio / UART downlink、通聯窗口自動化、或 on-orbit storage behavior
