# 測試紀錄：uart-hw-integration-v1

- 日期：2026-03-31
- 層級：L3 / L4
- 環境：
  - host: macOS development machine
  - target: Raspberry Pi 3 Model B+ via `ssh operator@youjun.local`
- 關聯變更：`openspec/changes/uart-hw-integration-v1/`
- 關聯文件：
  - `openspec/changes/uart-hw-integration-v1/specs/comm-subsystem/spec.md`
  - `openspec/changes/uart-hw-integration-v1/specs/verification-evidence/spec.md`

> Historical note (2026-05-27): this record predates the current host-role naming.
> Any `operator@youjun.local` reference below is historical evidence only.
> Current governed OBC target references are `operator@obc.local` or `operator@<private-lab-host>`.

## 1. 目標

本 change 的目標是把既有 external comm architecture 的第一條真實硬體 serial path 收斂成 governed flow：

- host 端以 `radio_mock_server --serial-device <host-tty>` 擔任 serial peer
- Raspberry Pi target 端以 `run_uart_stack.sh` / `run_rpi_uart_stack.sh` 對 explicit `COMM_DEVICE` 啟動 OBC stack
- controller layer 繼續沿用既有 `CommController` / `RadioController` / `UartDriver`
- 保持 TCP mock 與 PTY-backed validation gate 不回歸

## 2. 已完成的實作整理

- `radio_mock_server` 新增 `--serial-device /path/to/tty` 模式
- OBC runtime 新增正式 `--comm serial` 路徑，並保留 `pty` legacy alias
- shared comm transport 對外統一改成 generic serial-device 語意
- 新增 repo-local helper scripts：
  - `bash scripts/run_host_serial_peer.sh`
  - `bash scripts/run_uart_stack.sh`
  - `bash scripts/run_rpi_uart_stack.sh`
  - `bash scripts/run_rpi_uart_probe.sh`
- README、scripts README 與 narrative docs 已同步補入硬體 UART workflow

## 3. 本機回歸驗證

執行指令：

```bash
bash -n scripts/run_host_serial_peer.sh
bash -n scripts/run_uart_stack.sh
bash -n scripts/run_rpi_uart_stack.sh
bash -n scripts/run_rpi_uart_probe.sh
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

觀察摘要：

- 四個新增 helper scripts 的 shell syntax 檢查均通過
- shared verification gate 全數通過：
  - `01_generate`: PASS
  - `02_build`: PASS
  - `03_generate_ut`: PASS
  - `04_build_ut`: PASS
  - `05_check_all`: PASS
  - `06_openspec_validate_specs`: PASS

結論：

- 本次 serial-device 擴充未破壞既有 TCP mock / PTY 驗證基線
- OBC runtime、mock-radio backend 與 helper scripts 已具備正式硬體 probe 所需入口

## 4. Raspberry Pi ↔ Host 硬體 UART probe

執行指令：

```bash
RPI_SSH_TARGET=operator@<private-lab-host> \
bash scripts/run_rpi_uart_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0
```

裝置與啟動路徑：

- host serial device: `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
- target serial device: `/dev/serial0`
- host peer launch path: `radio_mock_server --serial-device /dev/cu.usbserial-$COMM_SERIAL_DEVICE`
- target launch path: `COMM_DEVICE=/dev/serial0 bash scripts/run_uart_stack.sh`
- probe helper 會在必要時暫停 `obc-installed-stack.service`，並在退出時自動恢復

觀察摘要：

- target runtime 明確顯示 `Comm mode: serial (/dev/serial0)`
- 初始 `status` 與 `radio status` 均顯示：
  - `uart connected=yes`
  - `radioLink connected=yes`
  - `radio enabled=no`
- `radio enable on` 成功，輸出 `radio enable response=0`
- runtime event 顯示 `RADIO_POWERED_ON`
- 後續 `radio status` 顯示：
  - `radio enabled=yes`
  - `power=10`
  - `freq=437000000`
  - `rssi=-68`
- `uart raw STATUS` 成功回傳：

```text
uart response: STATUS enabled=1 power=10 freq=437000000 temp=32.5 rssi=-68
```

- link 統計在整個 probe 過程維持 `connected=yes`
- probe 期間曾觀察到一次早期 `rxErr=1`，但之後 `radio enable`、`radio status`、`uart raw STATUS` 都可重複成功完成，未再出現 link 中斷

本次修正重點：

- `radio` 控制路徑與 `uart raw` 現在共用同一個 byte-stream transport，而不是各自重新打開同一個 serial device
- shared transport 內部已序列化 `connect` / `exchange` / `disconnect`，避免 scheduler 與 operator runtime 在真實 UART 上互相踩線

結論：

- `PASS`：Raspberry Pi ↔ development-host 的 governed UART/RS485 硬體鏈路已可正式支援既有 `CommController` / `RadioController` / `UartDriver` stack
- 本 change 清除了「Pi ↔ host serial hardware path」這個 `Blocked-HW`

剩餘 `Blocked-HW`：

- 真實 radio / vendor protocol 尚未接入
- 其他尚未驗證的電氣變體仍不在本次結論內，例如不同 transceiver 板型或 GPIO-level wiring permutations
