# 測試紀錄：radio-protocol-adapter-v1

- 日期：2026-04-01
- 層級：L2 / L3
- 環境：
  - host: macOS development machine
  - build: `build-fprime-automatic-native`
- 關聯變更：`openspec/changes/radio-protocol-adapter-v1/`
- 關聯文件：
  - `openspec/changes/radio-protocol-adapter-v1/specs/comm-subsystem/spec.md`
  - `openspec/changes/radio-protocol-adapter-v1/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是把目前 `RadioController` 下方的 hosted text framing 抽成正式的 radio protocol adapter layer，並保留：

- controller-layer API 與 telemetry / event semantics 不變
- current default adapter `mock-text`
- TCP mock、PTY / serial-device、以及 Raspberry Pi UART 路徑的既有 comm baseline 不回歸

## 2. 已完成的實作整理

- `RadioTransport` 新增 explicit `IRadioProtocolAdapter`
- 現有 hosted text request/response protocol 被命名為預設 adapter `mock-text`
- runtime 新增 `--radio-protocol <name>`，目前支援 `mock-text`
- helper scripts 新增 `RADIO_PROTOCOL` 環境變數入口，預設仍為 `mock-text`
- narrative docs 與 project README 已補入 adapter layer 與未來 KISS / vendor extension point

## 3. 驗證指令

### 3.1 Runtime option smoke check

```bash
build-fprime-automatic-native/bin/Darwin/OBC --help
```

觀察摘要：

- usage 文字已明確包含 `--radio-protocol mock-text`

### 3.2 Shared regression gate

```bash
bash -n scripts/run_dev_stack.sh scripts/run_uart_stack.sh scripts/run_rpi_uart_stack.sh scripts/run_rpi_uart_probe.sh scripts/run_rpi_boot_probe.sh packaging/rpi/launch/run_stack.sh
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

觀察摘要：

- 所有受影響 helper / launch scripts 的 shell syntax 檢查均通過
- `01_generate`: PASS
- `02_build`: PASS
- `03_generate_ut`: PASS
- `04_build_ut`: PASS
- `05_check_all`: PASS
- `06_openspec_validate_specs`: PASS

判讀重點：

- `05_check_all` 覆蓋 comm integration regression，因此代表 default adapter `mock-text` 仍與現有 comm validation baseline 相容
- adapter 抽取後，既有 comm stack 不需改寫 controller contract 即可通過 baseline 驗證

## 4. 結論

- `PASS`：radio framing 已正式抽成 adapter layer，且目前預設 adapter `mock-text` 的行為與既有 comm baseline 相容
- 本 change 只完成 adapter groundwork，不宣稱真實 KISS 或 vendor-specific radio protocol 已完成

## 5. 仍保留的範圍邊界

- `Blocked-HW`: 真實 radio / vendor protocol 尚未驗證
- `Blocked-HW`: KISS adapter 仍未實作
- `Blocked-HW`: 與特定 radio 硬體相關的 GPIO / I2C / power-sequencing 仍不在本次結論內
