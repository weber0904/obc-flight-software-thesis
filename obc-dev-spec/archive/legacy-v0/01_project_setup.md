# 01 — 專案建立與整合基線

## 1. 文件目的

本文件定義第一版 CubeSat OBC FSW 的專案建立方式、F' 專案骨架、開發 profile、Raspberry Pi 3B+ 目標模式，以及 libcsp / 模擬器 / GDS 的整合基線。

## 2. 唯一事實來源

若本文件與實際工具行為不一致，應以以下來源為準：

1. F' 官方文件 v4.1.0
2. 專案內實際 bootstrap 產生結果
3. 本文件集其餘章節的已定案一致性規則

## 3. 專案初始化策略

第一版採用官方 `fprime-bootstrap` 流程，不保留舊式手動 `git init + git submodule add` 作為正式路徑。

### 3.1 建立新專案

```bash
brew install pipx
pipx ensurepath
pipx install fprime-bootstrap
fprime-bootstrap project
```

建立完成後：

```bash
cd <project-root>
. fprime-venv/bin/activate
```

### 3.2 基線原則

- 必須使用 bootstrap 建立的虛擬環境
- F' 工具版本應與專案內依賴一致
- 專案骨架以 bootstrap 產生內容為起點，不反向仿造舊版手刻結構

### 3.3 baseline 最小操作序列

以下序列作為第一版最小可重現流程（以 `dev-macos` 為預設）：

```bash
cd <project-root>
. fprime-venv/bin/activate
fprime-util generate
fprime-util build
```

補充規則：

- `dev-macos` 與 `integ-rpi` 僅允許在設定/instances 層有差異。
- 不得為 profile 複製兩套業務邏輯。
- 若需切換外部鏈路（TCP mock / UART），必須透過配置檔或 deployment instance 選擇完成。

## 4. 開發 profile 設計

### 4.1 profile 定義

| profile | 目的 | OBC 運行位置 | 內部 CSP | 外部鏈路 |
|---------|------|--------------|----------|----------|
| `dev-macos` | 純軟體開發驗證 | macOS | ZMQ | TCP mock |
| `integ-rpi` | 目標平台整合 | Raspberry Pi 3B+ | ZMQ | TCP mock 或 UART |
| `flight-hw-future` | 未來實體硬體 profile | Raspberry Pi 3B+ | ZMQ 或後續 CAN | UART / radio vendor protocol |

### 4.2 設計要求

- 同一套應用邏輯不得複製成兩份 profile 專屬版本
- profile 差異應落在設定、instance 選擇、device path 與網路端點
- 若某項差異無法透過設定消化，需先在設計文件中補充理由

## 5. 專案目錄基線

```text
<project-root>/
├── CMakeLists.txt
├── settings.ini
├── fprime-venv/
├── OBC/
│   ├── CMakeLists.txt
│   ├── Ports/
│   ├── Types/
│   ├── Components/
│   └── Deployment/
├── simulators/
├── scripts/
├── docs/
└── .github/
```

### 5.1 OBC 目錄角色

- `Ports/`：專案共用 port 定義
- `Types/`：共用 enum / struct / alias
- `Components/`：F' 元件實作
- `Deployment/`：各 profile 的 topology / instances / 平台設定

### 5.2 simulators 目錄角色

- `simulators/common/`：OBC 與模擬器共享封包結構與常數
- `simulators/eps/`：EPS 模擬器
- `simulators/adcs/`：ADCS 模擬器
- 如需最小回音節點，可加入 `simulators/echo/`

## 6. F' 部署基線

### 6.1 最小部署需求

第一版 OBC deployment 至少應包含：

- C&DH 子拓樸
- 通訊子拓樸
- FileHandling 子拓樸
- `CspBridge`
- `EpsBridge`
- `AdcsBridge`
- `CommController`
- `BootManager`

### 6.2 profile 差異落點

| 項目 | `dev-macos` | `integ-rpi` |
|------|-------------|-------------|
| `comDriver` | `Drv::TcpClient` | `Drv::TcpClient` 或 `UartDriver` |
| device path | loopback / localhost | `/dev/tty*` 或指定端點 |
| CSP endpoint | localhost ZMQ proxy | localhost 或遠端 ZMQ proxy |
| GDS | macOS browser + CLI | macOS browser 或遠端操作 |

## 7. libcsp 整合原則

### 7.1 第一版整合目標

- OBC 透過 `CspBridge` 使用 libcsp
- EPS / ADCS 模擬器作為獨立 CSP node
- 內部 transport 以 ZMQ 為主
- CAN 僅列為未來擴充，不納入第一版驗收基準

### 7.2 ZMQ 使用原則

- 使用 ZMQ hub / proxy 模式
- macOS 預設避開 AirPlay 常見衝突 port
- endpoint 一律配置化，不寫死在程式碼中

建議預設：

| 服務 | 預設值 |
|------|--------|
| ZMQ subscribe port | `6100` |
| ZMQ publish port | `7100` |
| GDS GUI port | `8080` |

## 8. 模擬器與 GDS 整合基線

### 8.1 啟動順序

1. 啟動 ZMQ proxy
2. 啟動 EPS / ADCS / echo 模擬器
3. 啟動 OBC deployment
4. 啟動 GDS

操作細節見 [start-gds.md](./start-gds.md)。

### 8.2 驗證重點

- GDS 可載入字典並看到基本遙測
- `CSP_INIT` 可成功初始化
- `CSP_PING` 可打到模擬節點
- 基本 command / tlm / event path 可在 `dev-macos` 完成

### 8.3 baseline 驗證命令（建議）

```bash
# 檢查 OpenSpec 變更流程是否可用
openspec status --change "<change-name>"
openspec instructions apply --change "<change-name>" --json
```

```bash
# 檢查 F' 基線可建置（於 fprime-venv 內）
fprime-util generate
fprime-util build
```

## 9. 交叉編譯與 Raspberry Pi 目標模式

第一版要求文件層面保留 Raspberry Pi 3B+ 目標模式，但不要求本版文件定義完整裸機 toolchain。

### 9.1 必要要求

- 必須能描述如何切換到 Raspberry Pi 目標模式
- 必須明確指出哪些項目僅能在目標機實測
- 若使用 cross-compile，應以官方 F' cross-compilation 文件為準

### 9.2 非必要要求

- 不要求在本文件中鎖定唯一 toolchain 檔名
- 不要求在本文件中複製整份官方交叉編譯教學

## 10. 設計輸出

本章完成後，專案應具備以下設計輸出：

1. 以 `fprime-bootstrap` 建立的專案骨架
2. `dev-macos` / `integ-rpi` profile 定義
3. ZMQ 與 GDS 的啟動基線
4. OBC、simulators、scripts、docs 的目錄責任邊界

## 11. 驗收準則

| 編號 | 驗收項目 | 通過條件 |
|------|---------|---------|
| 1 | 專案初始化路徑一致 | 全文僅使用官方 bootstrap 流程 |
| 2 | profile 定義一致 | `dev-macos` 與 `integ-rpi` 差異清楚且不分叉主邏輯 |
| 3 | CSP 整合基線清楚 | 內部 CSP transport 明確定義為 ZMQ |
| 4 | GDS 啟動流程可追蹤 | 啟動順序與參數由 [start-gds.md](./start-gds.md) 補足 |
| 5 | 與其餘章節不衝突 | 不含 STM32 / raw flash 內容 |

## 12. 風險與回退

| 風險 | 說明 | 回退策略 |
|------|------|---------|
| F' 工具版本差異 | bootstrap 與本地 pip 版本不一致 | 以專案虛擬環境重新安裝 |
| ZMQ port 衝突 | macOS 常見保留 port 衝突 | 改用 6100 / 7100 / 8080 |
| profile 差異膨脹 | 開發與目標模式邏輯分叉 | 將差異退回設定層與 instance 選擇 |
| 目標機驗證延後 | 無法立即在 RPi 上完整實測 | 於測試文件標記 `Blocked-HW` 或 `Deferred-RPi` |
