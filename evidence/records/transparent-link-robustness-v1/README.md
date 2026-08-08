# 測試紀錄：transparent-link-robustness-v1

- 日期：2026-04-01
- 層級：L2 / L3 / L4
- 環境：
  - host: macOS development machine
  - target: Raspberry Pi 3 Model B+ via `ssh operator@<private-lab-host>`
- 關聯變更：`openspec/changes/transparent-link-robustness-v1/`
- 關聯文件：
  - `openspec/changes/transparent-link-robustness-v1/specs/comm-subsystem/spec.md`
  - `openspec/changes/transparent-link-robustness-v1/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是在既有 `transparent-link-frame-v1` 之上，補齊第一版 robustness evidence：

- repeated framed payload exchange
- host peer interruption / restart
- post-restart framed exchange recovery

本次仍然**不**處理：

- direct `fprime-gds` gateway
- 真實 RF over-the-air 行為
- legacy EnduroSat ESPS control/configuration plane
- newer `csp-es` / CSP-over-CAN 路線

## 2. 本次實作重點

- `radio_mock_server` 新增 `--max-framed-exchanges`，可用來受控地在 framed mode 跑固定筆數後結束 session
- host framed peer 會記錄：
  - `session-start`
  - `frame=<n> rx-bytes=<n> hex=<payload-hex>`
  - `session-end frames=<n> reason=<...>`
- 新增 `run_rpi_endurosat_framed_robustness_probe.sh`
  - phase 1：啟動 host framed peer，限制最多處理 3 筆 frame
  - phase 2：在 host peer 結束後重啟 framed peer
  - target 端在同一條 governed UART/RS485 path 上持續送 framed payload
  - 觀察 downtime 期間的 `uart framed exchange failed`
  - 觀察 restart 後重新拿到 framed response

## 3. 驗證指令

### 3.1 本機回歸

```bash
bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local
```

觀察摘要：

- `01_generate`: PASS
- `02_build`: PASS
- `03_generate_ut`: PASS
- `04_build_ut`: PASS
- `05_check_all`: PASS
- `06_openspec_validate_specs`: PASS

其中 `comm_transport_integration_test` 這次新增覆蓋：

- repeated framed payload exchange on a shared link
- framed path restart-oriented regression using fresh framed peers across repeated sessions

### 3.2 Raspberry Pi ↔ host robustness probe

```bash
RPI_SSH_TARGET=operator@<private-lab-host> \
bash scripts/run_rpi_endurosat_framed_robustness_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0
```

裝置與啟動路徑：

- host serial device: `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
- target serial device: `/dev/serial0`
- host peer mode:
  - `radio_mock_server --serial-device /dev/cu.usbserial-$COMM_SERIAL_DEVICE --mode transparent-framed-echo --baudrate 230400`
- phase 1 host peer limit:
  - `--max-framed-exchanges 3`
- target launch path:
  - `COMM_DEVICE=/dev/serial0`
  - `COMM_BAUDRATE=230400`
  - `RADIO_PROTOCOL=transparent-passive`
  - `bash scripts/run_uart_stack.sh`

## 4. 觀察結果

### 4.1 Phase 1：sustained framed exchange

host peer log：

```text
transparent-framed-echo session-start max-frames=3
transparent-framed-echo frame=1 rx-bytes=14 hex=454E4455524F5341545F50494E47
transparent-framed-echo frame=2 rx-bytes=8 hex=007E7D414243FF10
transparent-framed-echo frame=3 rx-bytes=15 hex=0102030405060708090A0B0C0D0E0F
transparent-framed-echo session-end frames=3 reason=max-frames
```

判讀：

- framed transparent path 可在同一條 governed serial session 上處理多筆 binary-safe payload
- reserved bytes 與 arbitrary binary bytes 仍能穩定通過

### 4.2 Phase 2：disconnect / restart / recovery

host phase 2 log：

```text
transparent-framed-echo session-start max-frames=0
transparent-framed-echo frame=1 rx-bytes=6 hex=DEADBEEF0011
transparent-framed-echo frame=2 rx-bytes=8 hex=A1B2C3D4E5F60708
transparent-framed-echo frame=3 rx-bytes=8 hex=5566778899AABBCC
```

target highlights：

```text
uart framed exchange failed
uart frame response hex: DEADBEEF0011
uart frame response hex: A1B2C3D4E5F60708
```

判讀重點：

- host peer 被受控中斷後，target 在 downtime 期間確實觀察到 `uart framed exchange failed`
- host peer 重啟後，target 重新拿到 framed response
- 實際哪一筆 payload 在 host restart 後先被 target 重新觀察到，可能受 serial buffering 與 timing 影響；因此本次 acceptance 以「downtime 有明確失敗」與「restart 後重新成功」為準，而不把某單一 payload 綁成唯一判據

## 5. 驗收結論

- `PASS`：framed transparent UART/RS485 path 已完成 repeated exchange evidence
- `PASS`：governed host-peer interruption 期間，target 端可觀察到 framed exchange failure
- `PASS`：host-peer restart 後，target 端可重新取得 framed response
- `PASS`：本 change 未破壞既有 direct TCP/GDS baseline、`mock-text` baseline 或 framed-link v1 format

## 6. 邊界與限制

- 這次 `PASS` 不等同：
  - ground-station integration 完成
  - RF behavior 完成
  - EnduroSat legacy control/configuration plane 完成
  - newer `csp-es` integration 完成
- serial buffering 與 timing 可能影響「restart 後第一筆成功 payload」的具體順序，因此 evidence 以 phase logs 與 target-side recovery outcome 綜合判讀
