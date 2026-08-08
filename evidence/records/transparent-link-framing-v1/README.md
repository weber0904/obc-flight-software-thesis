# 測試紀錄：transparent-link-framing-v1

- 日期：2026-04-01
- 層級：L2 / L3 / L4
- 環境：
  - host: macOS development machine
  - target: Raspberry Pi 3 Model B+ via `ssh operator@<private-lab-host>`
- 關聯變更：`openspec/changes/transparent-link-framing-v1/`
- 關聯文件：
  - `openspec/changes/transparent-link-framing-v1/specs/comm-subsystem/spec.md`
  - `openspec/changes/transparent-link-framing-v1/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是把既有 legacy EnduroSat transparent-UART data path 往前推一層，加入 repository-owned framing / deframing：

- 保留原本 direct TCP ↔ `fprime-gds` baseline 不動
- 保留原本 `mock-text` controller-oriented radio baseline 不動
- 在 transparent UART/RS485 path 上新增 binary-safe frame layer
- 讓 host peer 能對 framed payload 做 deframe / validate / reframe

## 2. Frame v1 定義

本次收斂的 transparent link frame v1 採用：

- trailing frame delimiter：`0x7E`
- escape byte：`0x7D`
- version byte：`0x01`
- flags byte：`0x00` for current governed probes
- payload length：16-bit big-endian
- payload：binary-safe，允許包含 `0x00`、`0x7E`、`0x7D`
- integrity：CRC-32 over `version + flags + length + payload`

escape 規則：

- `0x7E -> 0x7D 0x5E`
- `0x7D -> 0x7D 0x5D`

這個 framing 是 repository-owned link layer，不等同：

- `fprime-gds` ground TCP/framing
- `mock-text` radio adapter
- EnduroSat legacy control/configuration protocol
- 真實 RF over-the-air frame

## 3. 已完成的實作整理

- 新增 `TransparentLinkFraming` utility，提供 encode / decode / CRC-32 validation
- `ByteStreamTransport` 新增 delimiter-aware exchange path，不破壞原本 line-based `exchange()`
- `UartDriver` 新增 `exchangeDelimitedForRuntime()` / `exchangeDelimitedForTest()`
- `OBC` runtime 新增 `uart frame-hex <hex-payload>` operator command
- `radio_mock_server` 新增 `transparent-framed-echo` mode
- `run_rpi_endurosat_framed_probe.sh` 新增 governed Pi ↔ host framed probe
- host peer framed mode 會記錄 decoded payload hex，作為 UART framed exchange reviewable evidence

## 4. 驗證指令

### 4.1 本機回歸

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

本機 gate 同時覆蓋：

- `TransparentLinkFraming` codec path
- `radio_mock_server` framed peer integration test
- 既有 `mock-text` / TCP / PTY baseline regression

### 4.2 Raspberry Pi ↔ host framed transparent probe

```bash
RPI_SSH_TARGET=operator@<private-lab-host> \
bash scripts/run_rpi_endurosat_framed_probe.sh /dev/cu.usbserial-$COMM_SERIAL_DEVICE /dev/serial0
```

裝置與啟動路徑：

- host serial device: `/dev/cu.usbserial-$COMM_SERIAL_DEVICE`
- target serial device: `/dev/serial0`
- host peer mode:
  - `radio_mock_server --serial-device /dev/cu.usbserial-$COMM_SERIAL_DEVICE --mode transparent-framed-echo --baudrate 230400`
- target launch path:
  - `COMM_DEVICE=/dev/serial0`
  - `COMM_BAUDRATE=230400`
  - `RADIO_PROTOCOL=transparent-passive`
  - `bash scripts/run_uart_stack.sh`

觀察摘要：

- target runtime 顯示：
  - `Comm mode: serial (/dev/serial0 @230400)`
  - `Radio protocol: transparent-passive`
- `UART_OPEN` event 正常出現
- 下列 framed payload 皆成功往返：

```text
uart frame response hex: 454E4455524F5341545F50494E47
uart frame response ascii: ENDUROSAT_PING

uart frame response hex: 007E7D414243FF10
uart frame response ascii: .~}ABC..

uart frame response hex: 0102030405060708090A0B0C0D0E0F
uart frame response ascii: ...............
```

- host peer log 同步觀察到三筆 decoded payload：

```text
transparent-framed-echo rx-bytes=14 hex=454E4455524F5341545F50494E47
transparent-framed-echo rx-bytes=8 hex=007E7D414243FF10
transparent-framed-echo rx-bytes=15 hex=0102030405060708090A0B0C0D0E0F
```

判讀重點：

- `0x00`、`0x7E`、`0x7D` 保留字節可正確通過 link framing
- host peer 已能 deframe、validate CRC、reframe 回傳
- 這次 `PASS` 只宣稱 transparent UART/RS485 framed data path 成立

## 5. Raw 與 Framed 的區別

目前 repo 內的 transparent path 分成兩條：

- `uart raw`
  - 用來驗證純 transparent payload transport
  - 不提供 frame boundary / CRC / binary-safe escaping
- `uart frame-hex`
  - 用來驗證 repository-owned transparent frame v1
  - 提供 delimiter、escaping、length、CRC-32

兩者都不是 ground-station integration，也不是 vendor control plane。

## 6. 結論

- `PASS`：transparent link frame v1 已在既有 Raspberry Pi ↔ host UART/RS485 governed path 上完成 binary-safe framed exchange
- `PASS`：direct TCP/GDS baseline 與 `mock-text` controller baseline 仍維持原有驗證路徑，未被本 change 取代
- `PASS`：host peer 已可作為 governed framed deframe / reframe peer

## 7. 剩餘邊界

- Out of scope: direct transparent path ↔ `fprime-gds` gateway
- Out of scope: 真實 RF over-the-air 行為、調變、解調
- Out of scope: EnduroSat legacy ESPS control/configuration plane
- Out of scope: newer `csp-es` / CSP-over-CAN hardware generation
- `Blocked-HW`: 真實 EnduroSat radio 硬體尚未接入本 framed validation flow
