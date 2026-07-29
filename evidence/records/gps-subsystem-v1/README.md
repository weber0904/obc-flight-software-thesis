# 測試紀錄：gps-subsystem-v1

- 日期：2026-04-06
- 層級：L2 / L3
- 環境：
  - host: macOS development machine
  - mode: hosted fake/replay validation
- 關聯變更：`openspec/changes/archive/2026-04-06-gps-subsystem-v1/`
- 關聯文件：
  - `openspec/changes/archive/2026-04-06-gps-subsystem-v1/specs/gps-subsystem/spec.md`
  - `openspec/changes/archive/2026-04-06-gps-subsystem-v1/specs/housekeeping-archive/spec.md`
  - `openspec/changes/archive/2026-04-06-gps-subsystem-v1/specs/verification-evidence/spec.md`
  - `evidence/verification-path-registry.md`

## 1. 目標

本 change 的目標是在不先碰 Raspberry Pi UART 接線的前提下，建立第一版 GPS 子系統：

- 不把 GPS 混進 `comm-subsystem`
- 先以 fake / replay sentence source 驅動 hosted GPS path
- 新增 bounded NMEA parser 與 cached GPS state
- 讓 GPS telemetry 能被 housekeeping archive 一起收錄

本次仍然**不**處理：

- Raspberry Pi GPIO UART wiring / live hardware bring-up
- live-sky GPS fix reception claim
- PPS / precise time sync
- scheduler、pass-op、或 ADCS policy coupling

## 2. 本次實作重點

- 新增 `simulators/gps/`
  - `NmeaParser.*`
  - `GpsSource.*`
  - `GpsTypes.hpp`
- 新增 `OBC/Components/GpsBridge/`
  - repository-owned `GpsBridge`
  - fake / replay source mode
  - `GPS_*` telemetry、event、command
  - cached runtime accessor
- `HousekeepingSnapshotProvider` / `HousekeepingArchiveStore`
  - 新增 GPS cached state capture 與序列化
- 新增：
  - `gps_support_unit_test`
  - `gps_bridge_cached_state_integration_test`
  - `housekeeping_archive_gps_integration_test`
  - `scripts/run_gps_hosted_probe.sh`

## 3. 驗證指令

### 3.1 跑完整本機 verification gate

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

### 3.2 直接執行 GPS parser/support unit test

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/gps_support_unit_test
```

### 3.3 直接執行 GPS bridge cached-state integration test

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/gps_bridge_cached_state_integration_test
```

### 3.4 直接執行 GPS housekeeping archive integration test

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/housekeeping_archive_gps_integration_test
```

### 3.5 執行 repository-owned hosted GPS probe

```bash
bash scripts/run_gps_hosted_probe.sh
```

## 4. 觀察結果

### 4.1 GPS source model

- v1 GPS source mode 有兩種：
  - `fake`
  - `replay`
- `replay` 藉由 `OBC_GPS_REPLAY_FILE` 指向 sentence file
- 若 replay file 不可用，runtime 仍可留在 `fake`

### 4.2 Parser behavior

- 支援第一版 bounded NMEA subset：
  - `GGA`
  - `RMC`
- 可區分：
  - `OK`
  - `NO_FIX`
  - `INVALID_CHECKSUM`
  - `MALFORMED`
  - `UNSUPPORTED`

### 4.3 Cached-state behavior

- valid fix 會更新：
  - `fixValid`
  - latitude / longitude
  - altitude
  - speed / course
  - satellite count
  - HDOP
  - UTC time / date
- no-fix sample 會保留 `hasSample=yes`，但把 `fixValid=no`
- malformed / invalid sentence 不會覆寫最後一份有效 cached state，但會增加 rejected count

### 4.4 Housekeeping archive integration

- `HousekeepingSnapshotProvider` 已能收集 GPS cached state
- `HousekeepingArchiveStore` 已能把 GPS cached state 序列化進 archive record
- GPS archive capture 目前是沿用既有 archive snapshot/capture path，不額外引入第二條 transport poll

## 5. 測試結果

- `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`: `PASS`
  - 包含 `fprime-util generate/build`
  - 包含 `fprime-util generate --ut/build --ut`
  - 包含 `fprime-util check --all`
  - 包含 `openspec validate --specs`
- `gps_support_unit_test`: `PASS`
  - valid `GGA/RMC`
  - no-fix
  - invalid checksum
  - replay source cycling
- `gps_bridge_cached_state_integration_test`: `PASS`
  - valid fix 進 cached state
  - no-fix sample 清除 valid-fix 狀態
  - malformed / invalid sentence 累加 rejected count 且不覆寫最後 cached state
- `housekeeping_archive_gps_integration_test`: `PASS`
  - GPS cached state 會被 serialise 進 archive record
- `run_gps_hosted_probe.sh`: `PASS`
  - hosted replay 模式下可觀察 `GPS_FIX_ACQUIRED`
  - hosted replay 模式下可觀察 `GPS_FIX_LOST`
  - hosted replay 模式下可觀察 `GPS_PARSE_ERROR`
  - hosted runtime 會同時產生 `HK_CAPTURE_RECORDED` 與 `runtime-root/hk/` 產物

## 6. 驗收結論

- `PASS`：第一版 GPS 子系統已建立，且保持為獨立於 `comm-subsystem` 的能力切片
- `PASS`：GPS fake / replay path 已能支撐 hosted validation
- `PASS`：GPS cached telemetry 已可納入 housekeeping archive
- `PASS`：本次未破壞既有 simulator integration、autonomy、comm、或 archive regression baseline

## 7. 邊界與限制

- 目前只有 hosted fake/replay evidence
- `Blocked-HW`：尚未做 Raspberry Pi GPIO UART wiring、live module bring-up、或 live-sky fix 驗證
- 目前只做 bounded NMEA subset，尚未做完整 sentence coverage
- 目前未處理 PPS / precise timing、scheduler coupling、或 mission-level consumers
