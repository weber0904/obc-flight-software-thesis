# 05 — 通訊子系統與外部鏈路

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/comm-subsystem/spec.md`](../openspec/specs/comm-subsystem/spec.md)。

## 1. 文件角色

本文件定義目前 active baseline 的 OBC 外部通訊鏈路、`CommController`、`UartDriver`、`RadioController` family、相關 owner 邊界與驗證策略。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/06_comm_subsystem.md` | 分層原則、driver family、驗證模式、KISS 立場 |
| `archive/legacy-v0/02_command_telemetry_design.md` | `COMM_*`、`RADIO_*`、`UART_*` 契約 |

## 3. 分層原則

本 capability 僅處理 **外部通訊鏈路**。以下不屬於本文件定義範圍：

- OBC ↔ EPS / ADCS 的內部 CSP network
- CAN first-version 內部節點互連

第一版外部鏈路策略：

| 階段 | transport | 說明 |
|---|---|---|
| 開發期 | TCP mock | 使用 `Drv::TcpClient` 驗證完整收發路徑 |
| 目標整合期 | serial device path | 使用通用 `UartDriver`，可指向 PTY 或真實 tty |
| 協定層預設 | `mock-text` adapter | 第一版 hosted mock-radio framing，保持可替換 |
| transparent data path | `transparent-passive` + optional frame layer | legacy EnduroSat 資料面驗證，raw 與 framed path 分開治理 |
| 未來擴充 | UART/KISS 或 vendor adapter | 依真實 radio protocol 再定 |

## 4. 元件 family 與 instances

### 4.1 `UartDriver`

設計原則：

- 與 `Drv::ByteStreamDriverModel` 相容
- 使用單一 family 支援不同頻段
- 差異僅落在 device path、baud rate 與 line discipline

instance 範例：

```fpp
instance sbandUart: OBC.UartDriver base id 0x10055000
instance uhfUart:   OBC.UartDriver base id 0x10056000
```

### 4.2 `RadioController`

設計原則：

- 提供 radio power / frequency / status 控制
- command / telemetry naming 保持 generic
- 由 instance 決定實際硬體語意

instance 範例：

```fpp
instance sbandCtrl: OBC.RadioController base id 0x10057000
instance uhfCtrl:   OBC.RadioController base id 0x10058000
```

### 4.3 `CommController`

職責：

- 管理活動頻段
- 管理通訊窗口開始 / 結束
- 將 band selection 與 radio enable 狀態整理成可觀測 telemetry
- 在 active baseline 中，將 COMM subsystem responsiveness probe 轉成 owner-mediated async/cached semantics，而不是在 rate group 內直接同步 `ping()`

### 4.4 `CspRuntimeOwner` 與 `GroundLinkHealthProvider`

active baseline 補充 owner：

- `CspRuntimeOwner`：在 `TopCcsds` 內作為唯一 intended deployed libcsp runtime owner，負責 request serialization、timeout/coalesced observability，以及 COMM steady-state CSP client path
- `GroundLinkHealthProvider`：由 raw driver observation 推導 per-band availability/freshness；與 `GroundLinkDriver` / `CommController` 分開治理

## 5. 公開契約

### 5.1 Commands

| 指令名稱 | owner | 說明 |
|---|---|---|
| `COMM_SET_ACTIVE` | `CommController` | 選擇活動頻段 |
| `COMM_START_PASS` | `CommController` | 開始通訊窗口 |
| `COMM_STOP_PASS` | `CommController` | 結束通訊窗口 |
| `RADIO_ENABLE` | `RadioController` family | 啟用 / 停用 radio |
| `RADIO_SET_POWER` | `RadioController` family | 設定發射功率 |
| `RADIO_SET_FREQ` | `RadioController` family | 設定頻率 |
| `RADIO_GET_STATUS` | `RadioController` family | 讀取 radio 狀態 |

### 5.2 Telemetry

- `COMM_ACTIVE_BAND`
- `COMM_PASS_ACTIVE`
- `COMM_PASS_REMAINING`
- `COMM_TOTAL_PASSES`
- `UART_TX_BYTES`
- `UART_RX_BYTES`
- `UART_TX_ERRORS`
- `UART_RX_ERRORS`
- `UART_CONNECTED`
- `RADIO_ENABLED`
- `RADIO_TX_POWER`
- `RADIO_FREQ`
- `RADIO_TEMP`
- `RADIO_RSSI`

### 5.3 Events

- `COMM_BAND_SWITCH`
- `COMM_PASS_START`
- `COMM_PASS_END`
- `UART_OPEN`
- `UART_ERROR`
- `RADIO_POWERED_ON`
- `RADIO_POWERED_OFF`
- `RADIO_OVERTEMP`

## 6. KISS 與 transport 立場

第一版立場如下：

1. `UartDriver` 先提供 generic byte stream transport。
2. `RadioController` 下方應存在 protocol adapter 層，第一版預設為 `mock-text`。
3. 未取得真實 radio protocol 前，不把整體設計綁死在 KISS。
4. 若實體 radio 需要 KISS，應於 driver 上層或 adapter 層補入 framing，不回寫破壞通用 driver family。
5. legacy transparent-UART path 可在 `UartDriver` 上方疊加 repository-owned frame layer，但這不應取代 direct TCP/GDS baseline。
6. active baseline 的 deployed libcsp 使用不得再讓 `CommController` 或其他 steady-state client 各自 claim direct deployed `defaultRuntime()` ownership；COMM-side CSP steady-state work 應經 `CspRuntimeOwner`。

## 7. 驗證模式

### 7.1 開發期

- TCP mock 路徑必測
- GDS command / telemetry / event path 必須可通

### 7.2 半模擬 UART

可用 `socat` 建立 PTY pair 驗證：

```bash
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

用途：

- 將 `UartDriver` 接到一端 PTY
- 將 mock radio 或測試工具接到另一端 PTY
- 驗證 driver 初始化、收發與錯誤恢復

### 7.3 真實硬體

第一條應優先收斂的真實硬體路徑是：

- Raspberry Pi 3B+ OBC 以 `run_rpi_uart_stack.sh` / `run_rpi_uart_probe.sh` 對 explicit `COMM_DEVICE`
- libcsp integration base 驗證時，可用 `run_rpi_csp_comm_baseline_probe.sh` 在同一個 target run 中確認 internal CSP node reachability 與 external comm UART 收發，但兩者仍是分開的路徑
- 開發機以 `run_host_serial_peer.sh` 對 explicit host serial device path
- `CommController` / `RadioController` / `UartDriver` 不因 transport 切換而改寫 controller 邏輯
- 第一版驗證的 radio framing 仍是 `mock-text` adapter，不等同真實 vendor radio protocol 已完成
- Raspberry Pi `/dev/serial0` 在目前 baseline 中預設保留給 external comm/radio path；其他子系統不得在未經 governed allocation change 的情況下共用該 device

完成上述流程後，以下項目仍可保留 `Blocked-HW`：

- 真實 radio power / telemetry / GPIO / I2C 驗證
- GPS live UART，直到另有 USB-UART、secondary UART 或其他非衝突配置被正式批准

### 7.4 Transparent framed data path

當需要在 transparent-UART path 上驗證 binary-safe payload 時，第一版採 repository-owned framing：

- delimiter: `0x7E`
- escape byte: `0x7D`
- length field
- CRC-32

角色定位：

- `uart raw`: 驗證純 transparent payload 通道
- `uart frame-hex`: 驗證 transparent data path 上方的 frame / deframe / CRC 行為

下一個遞進層是 framed transparent robustness：

- repeated framed exchange
- host peer interruption / restart
- post-restart recovery

這一層的目的是先把 transparent data path 的穩定性補齊，再決定是否往 ground gateway、vendor control plane 或更完整 HIL flow 推進。

這一層不等同：

- `fprime-gds` 的 direct TCP ground path
- vendor-specific control/configuration plane
- RF / over-the-air framing
