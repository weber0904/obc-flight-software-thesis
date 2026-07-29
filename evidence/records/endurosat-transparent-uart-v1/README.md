# 測試紀錄：endurosat-transparent-uart-v1

- 日期：2026-04-01
- 層級：L3 / L4
- 環境：
  - host: macOS development machine
  - target: Raspberry Pi 3 Model B+ via `ssh operator@<private-lab-host>`
- 關聯變更：`openspec/changes/endurosat-transparent-uart-v1/`
- 關聯文件：
  - `openspec/changes/endurosat-transparent-uart-v1/specs/comm-subsystem/spec.md`
  - `openspec/changes/endurosat-transparent-uart-v1/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是收斂一條 governed 的 legacy EnduroSat S-band transparent-UART 驗證流程：

- 沿用既有 Raspberry Pi ↔ host UART/RS485 硬體鏈路
- host 端 peer 以 `radio_mock_server --mode transparent-echo` 模擬透明 serial 對端
- OBC 端沿用既有 `UartDriver` raw exchange path，不重寫 controller layer
- 將 legacy hardware 的 data path 與仍未完成的 control plane 明確分開

## 2. Mac 端模擬 peer 的定位

目前 host 端 `transparent-echo` peer 已足夠支援這次 change 的驗證目標，但它不是完整的 EnduroSat transceiver 模擬器。

已覆蓋的範圍：

- explicit serial device path
- legacy UART baudrate `230400`
- Raspberry Pi target 與 host peer 間的透明 payload 往返
- 避免 `RadioController` 在 transparent path 上發送既有 `mock-text` 狀態查詢

仍未覆蓋的範圍：

- ESPS control / configuration plane
- legacy `Pipe mode` / `Airmac` mode
- 任意 binary framing、timing 抖動、錯誤注入或 RF 行為
- newer `csp-es`-based hardware generation

因此本次 `PASS` 的意義是「透明 UART data path 已驗證」，不是「真實 EnduroSat radio 已完整整合」。

## 3. 已完成的實作整理

- `SerialByteStreamTransport` 新增 explicit `baudrate` 設定，並支援 legacy `230400`
- `radio_mock_server` 新增 `transparent-echo` mode 與 serial-device `baudrate` 設定
- 新增 `transparent-passive` radio protocol adapter，確保 transparent path 不再發送 `mock-text` 控制封包
- 新增 `bash scripts/run_rpi_endurosat_transparent_probe.sh`
- `sync_rpi_workspace.sh` 已補上 macOS metadata / xattr 抑制，避免 Pi sync 因 tar 警告中止

## 4. 驗證指令

### 4.1 本機回歸

```bash
bash -n scripts/sync_rpi_workspace.sh scripts/run_rpi_endurosat_transparent_probe.sh
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

觀察摘要：

- shell syntax 檢查通過
- shared verification gate 全數通過：
  - `01_generate`: PASS
  - `02_build`: PASS
  - `03_generate_ut`: PASS
  - `04_build_ut`: PASS
  - `05_check_all`: PASS
  - `06_openspec_validate_specs`: PASS

### 4.2 Raspberry Pi ↔ host transparent-UART probe

```bash
RPI_SSH_TARGET=operator@<private-lab-host> \
bash scripts/run_rpi_endurosat_transparent_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0
```

裝置與啟動路徑：

- host serial device: `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
- target serial device: `/dev/serial0`
- host peer launch path:
  - `radio_mock_server --serial-device /dev/cu.usbserial-$COMM_SERIAL_DEVICE --mode transparent-echo --baudrate 230400`
- target launch path:
  - `COMM_DEVICE=/dev/serial0`
  - `COMM_BAUDRATE=230400`
  - `RADIO_PROTOCOL=transparent-passive`
  - `bash scripts/run_uart_stack.sh`

觀察摘要：

- target runtime 明確顯示：
  - `Comm mode: serial (/dev/serial0 @230400)`
  - `Radio protocol: transparent-passive`
- `UART_OPEN` event 正常出現
- 下列 payload 均成功往返：

```text
uart response: ENDUROSAT_PING
uart response: S_BAND_FRAME_001
uart response: PAYLOAD-ALPHA-123
```

判讀重點：

- 這代表既有 Raspberry Pi ↔ host UART/RS485 governed path 可在 legacy baudrate 下傳遞 transparent payload
- 這次驗證刻意經由 `uart raw`，因此確認的是 data plane，不是 `RadioController` 的 command/status contract
- `transparent-passive` adapter 的存在，避免了 controller-layer scheduler 在透明鏈路上注入 `STATUS` 類控制文法

## 5. 結論

- `PASS`：legacy EnduroSat 風格的 transparent-UART data path 已透過 Raspberry Pi ↔ host 實體 serial link 收斂成 governed validation flow
- `PASS`：新增 transparent path 後，既有 comm baseline 與 shared verification gate 未回歸
- 本 change 不宣稱真實 EnduroSat S-band transceiver 的 control/configuration plane 已完成

## 6. 剩餘邊界

- `Blocked-HW`: 真實 EnduroSat S-band transceiver 尚未接入本 repository workflow，因此尚未驗證與實機的 end-to-end UART transparent path
- Out of scope: legacy ESPS control/configuration integration
- Out of scope: legacy `Pipe mode` / `Airmac` mode
- Out of scope: newer `csp-es`-based hardware generation
