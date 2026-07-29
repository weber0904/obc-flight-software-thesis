# 04 — EPS 模擬器與 `EpsBridge` 規格

## 1. 文件目的

本文件定義第一版 EPS 模擬器與 OBC 側 `EpsBridge` 的系統行為、封包介面、模型範圍與驗證要求。

## 2. 系統角色

| 元件 | 角色 |
|------|------|
| `simulators/eps/` | CSP Node 2，模擬電力系統 |
| `EpsBridge` | OBC 端代理，將 EPS 資料映射為 F' command / tlm / event |
| `CspBridge` | `EpsBridge` 與外部模擬器之間的傳輸橋接 |

## 3. 通訊基線

- Node ID：EPS = 2
- 第一版 transport：ZMQ
- OBC 與 EPS 間使用 CSP request / response 風格
- `EpsBridge` 預設採週期輪詢 + 事件告警模式

## 4. 模擬範圍

### 4.1 必含物理量

| 分類 | 欄位 |
|------|------|
| 電池 | 電壓、電流、SoC、溫度 |
| 太陽能板 | 電壓、電流、日照狀態 |
| PDU | 8 路開關狀態、輸出功率、過流旗標 |

### 4.2 非第一版重點

以下可簡化，不要求高保真：

- 詳細化學電池模型
- 真實軌道與姿態耦合照明模型
- 供電暫態與 EMI 細節

## 5. 模型規格

### 5.1 電池模型

```c
typedef struct {
    float voltage;
    float current;
    float soc;
    float temperature;
    float capacity_ah;
    float internal_r;
} battery_state_t;
```

設計約束：

- SoC 範圍必須限制於 `0 ~ 100%`
- 電壓範圍必須限制於合理 2S Li-ion 區間
- 溫度必須可觸發過溫事件

### 5.2 太陽能板模型

- 以 `illumination` 表示簡化日照狀態
- 允許用固定週期近似日蝕 / 日照切換
- 第一版只要求可區分：全日照、部分入射、日蝕

### 5.3 PDU 模型

- 至少 8 路輸出
- 支援 enable / disable
- 支援每路 nominal current draw
- 支援過流檢查與錯誤旗標

建議負載語意：

| Channel | 用途 |
|---------|------|
| 0 | OBC |
| 1 | ADCS |
| 2 | COM S-Band |
| 3 | COM UHF |
| 4 | Payload |
| 5 | Heater |
| 6 | Reserved |
| 7 | Reserved |

## 6. CSP 服務介面

### 6.1 Port 分配

| Port | 服務 |
|------|------|
| 1 | `EPS_SRV_STATUS` |
| 2 | `EPS_SRV_PDU` |
| 3 | `EPS_SRV_CONFIG` |
| 4 | `EPS_SRV_RESET` |

### 6.2 封包設計原則

- 單次狀態回應應可放入單一 CSP payload
- 所有封包必須帶 `seq` 以便 request / response 對應
- 結構應集中於 `simulators/common/protocol.h`

### 6.3 範例結構

```c
typedef struct __attribute__((packed)) {
    uint8_t seq;
    uint8_t status;
    float battery_v;
    float battery_i;
    float battery_soc;
    float battery_temp;
    float solar_v;
    float solar_i;
    float power_out;
    uint8_t pdu_status;
} eps_status_response_t;
```

## 7. `EpsBridge` 行為規格

### 7.1 command 映射

| F' 指令 | EPS 動作 |
|---------|---------|
| `EPS_GET_STATUS` | 查詢狀態 |
| `EPS_SET_PDU` | 切換配電通道 |
| `EPS_SET_HEATER` | 切換加熱器 |
| `EPS_RESET` | 重設 EPS 模擬器狀態 |

### 7.2 輪詢策略

- `schedIn` 定期送出狀態查詢
- 成功收到回應後更新 telemetry
- 若回應逾時，記錄 `EPS_COMM_ERROR`
- 若連續逾時達門檻，可上報 health degradation

### 7.3 `isConnected` / fallback 原則

若 CSP 尚未初始化或 EPS 無回應：

- 不輸出不合理隨機值
- 保留最後一次有效狀態或使用明確 default
- 透過 event 告知資料已降級

## 8. 遙測與事件對應

`EpsBridge` 必須至少覆蓋以下欄位：

- `EPS_VBAT`
- `EPS_IBAT`
- `EPS_SOC`
- `EPS_VSOLAR`
- `EPS_ISOLAR`
- `EPS_TEMP_BAT`
- `EPS_PDU_STATUS`
- `EPS_POWER_OUT`

事件至少包含：

- `EPS_STATUS_RECEIVED`
- `EPS_LOW_BATTERY`
- `EPS_CRITICAL_BATTERY`
- `EPS_OVERTEMP`
- `EPS_PDU_CHANGE`
- `EPS_COMM_ERROR`

## 9. 驗證策略

### 9.1 可在現況完成的驗證

| 驗證項目 | 方法 |
|---------|------|
| EPS 模擬器可獨立運行 | 啟動模擬器觀察狀態輸出 |
| ZMQ/CSP 通訊 | 使用測試節點或 script 查詢狀態 |
| `EpsBridge` telemetry | GDS 顯示對應通道 |
| 低電量事件 | 控制 SoC 下降至閾值 |
| PDU 切換 | 發送 command 後觀察狀態改變 |

### 9.2 不可在現況完成的驗證

| 項目 | 狀態 |
|------|------|
| 真實 EPS 硬體通訊 | `Blocked-HW` |
| 真實供電與熱行為量測 | `Blocked-HW` |

## 10. 驗收準則

| 編號 | 驗收項目 | 通過條件 |
|------|---------|---------|
| 1 | Node / port 一致 | 與主文件與 command 文件一致 |
| 2 | Bridge 行為一致 | `schedIn` 輪詢 + 通訊錯誤 fallback |
| 3 | 可在 ZMQ 模式驗證 | 不依賴 CAN 或實體線材 |
| 4 | 遙測與事件齊全 | 與 [02_command_telemetry_design.md](./02_command_telemetry_design.md) 一致 |
| 5 | `Blocked-HW` 有標記 | 未把真硬體測試列為本版必要條件 |
