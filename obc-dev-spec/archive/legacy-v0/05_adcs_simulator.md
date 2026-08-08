# 05 — ADCS 模擬器與 `AdcsBridge` 規格

## 1. 文件目的

本文件定義第一版 ADCS 模擬器、`AdcsBridge`、控制模式、感測器與驗證要求。

## 2. 系統角色

| 元件 | 角色 |
|------|------|
| `simulators/adcs/` | CSP Node 3，模擬姿態動力學與控制迴路 |
| `AdcsBridge` | OBC 端代理，將 ADCS 狀態映射成 F' command / tlm / event |
| `CspBridge` | 轉送 OBC 與 ADCS 間的 CSP 封包 |

## 3. 通訊基線

- Node ID：ADCS = 3
- 第一版 transport：ZMQ
- 使用 CSP request / response 模式
- `AdcsBridge` 預設以週期輪詢為主，必要時接受 on-demand 查詢

## 4. 模擬範圍

### 4.1 第一版必含能力

| 分類 | 內容 |
|------|------|
| 姿態狀態 | 四元數、角速度 |
| 感測器 | 磁力計、陀螺儀、太陽感測器（可簡化） |
| 致動器 | 反應輪、磁力矩器（模型化） |
| 控制模式 | `IDLE`、`DETUMBLE`、`POINTING` |

### 4.2 第一版可簡化項目

- IGRF 高保真模型
- 真實星敏感測器
- 完整軌道動力學耦合
- 高階容錯控制器

## 5. 動力學與感測器規格

### 5.1 姿態狀態

```c
typedef struct {
    double q[4];
    double omega[3];
    double I[3];
} attitude_state_t;
```

設計要求：

- 四元數範數應維持接近 1
- 角速度單位統一為 `rad/s`
- 第一版可使用固定 `dt` 積分

### 5.2 感測器模型

| 感測器 | 第一版要求 |
|--------|-----------|
| 磁力計 | 支援偏差與高斯雜訊 |
| 陀螺儀 | 支援偏差漂移與雜訊 |
| 太陽感測器 | 支援可見 / 不可見與近似方向向量 |

### 5.3 致動器模型

| 致動器 | 第一版要求 |
|--------|-----------|
| 反應輪 | 支援最大轉速與簡化力矩輸出 |
| 磁力矩器 | 支援偶極矩限制與 B-dot 控制耦合 |

## 6. 控制模式

### 6.1 `DETUMBLE`

- 使用 B-dot 類控制邏輯
- 驗收目標：角速度下降至設計門檻
- 事件：達標時觸發 `ADCS_DETUMBLE_COMPLETE`

### 6.2 `POINTING`

- 使用簡化 PD 指向控制
- 驗收目標：指向誤差持續低於門檻
- 事件：達標時觸發 `ADCS_POINTING_ACQUIRED`

### 6.3 `IDLE`

- 不進行主動控制
- 可保留感測器更新與狀態回報

## 7. CSP 服務介面

| Port | 服務 |
|------|------|
| 1 | `ADCS_SRV_ATTITUDE` |
| 2 | `ADCS_SRV_SENSORS` |
| 3 | `ADCS_SRV_CONTROL` |
| 4 | `ADCS_SRV_CONFIG` |

### 7.1 回應結構原則

- 回應應攜帶 mode、四元數、角速度與主要感測器值
- 結構定義集中於 `simulators/common/protocol.h`
- 第一版可使用 packed struct，但需明確版本控管

範例：

```c
typedef struct __attribute__((packed)) {
    uint8_t seq;
    uint8_t status;
    uint8_t mode;
    double q[4];
    float omega[3];
    float pointing_err;
    float mag[3];
    float sun_vec[3];
    float rw_speed[3];
} adcs_attitude_response_t;
```

## 8. `AdcsBridge` 行為規格

### 8.1 command 映射

| F' 指令 | ADCS 動作 |
|---------|-----------|
| `ADCS_SET_MODE` | 切換控制模式 |
| `ADCS_SET_TARGET` | 更新目標指向 |
| `ADCS_GET_ATTITUDE` | 查詢姿態 |
| `ADCS_CALIBRATE` | 執行對應感測器校準 |

### 8.2 輪詢策略

- `schedIn` 定期查詢姿態與主要感測器狀態
- 收到有效回應後更新 telemetry
- 無回應時觸發 `ADCS_COMM_ERROR` 或等價事件

### 8.3 fallback 原則

- 無效回應不可直接覆蓋為隨機數值
- 保留最後一次有效狀態並標記資料降級
- 感測器欄位若失效，需觸發 `ADCS_SENSOR_FAULT`

## 9. 遙測與事件對應

`AdcsBridge` 最少應對應：

- `ADCS_Q0` ~ `ADCS_Q3`
- `ADCS_OMEGA_X` ~ `ADCS_OMEGA_Z`
- `ADCS_MAG_X` ~ `ADCS_MAG_Z`
- `ADCS_MODE`
- `ADCS_POINTING_ERR`

事件至少包含：

- `ADCS_MODE_CHANGE`
- `ADCS_DETUMBLE_COMPLETE`
- `ADCS_POINTING_ACQUIRED`
- `ADCS_SENSOR_FAULT`
- `ADCS_COMM_ERROR`

## 10. 驗證策略

### 10.1 可在現況完成的驗證

| 驗證項目 | 方法 |
|---------|------|
| 姿態積分穩定 | 單元測試驗證四元數範數 |
| 去翻滾收斂 | 指定初始角速度後觀察下降 |
| 指向誤差下降 | 設定目標後觀察 `ADCS_POINTING_ERR` |
| ZMQ/CSP 通訊 | 透過模擬器 + OBC + GDS 驗證 |
| `AdcsBridge` telemetry | 在 GDS 中確認欄位可見 |

### 10.2 不可在現況完成的驗證

| 項目 | 狀態 |
|------|------|
| 真實 ADCS 硬體閉迴路 | `Blocked-HW` |
| 真實感測器標定 | `Blocked-HW` |

## 11. 驗收準則

| 編號 | 驗收項目 | 通過條件 |
|------|---------|---------|
| 1 | 模式定義一致 | `IDLE / DETUMBLE / POINTING` 與型別文件一致 |
| 2 | Bridge 行為一致 | 採輪詢 + fallback + event 通報 |
| 3 | 內部 transport 一致 | 全文使用 ZMQ，不將 CAN 視為第一版必要條件 |
| 4 | Telemetry / event 一致 | 與 [02_command_telemetry_design.md](./02_command_telemetry_design.md) 相符 |
| 5 | `Blocked-HW` 清楚 | 真實硬體驗證不列為第一版必要門檻 |
