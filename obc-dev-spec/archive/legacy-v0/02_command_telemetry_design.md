# 02 — 指令、遙測、事件與型別設計

## 1. 文件目的

本文件定義第一版 OBC Flight Software 的 command、telemetry、event 與共用型別。設計原則如下：

1. F' 字典應能直接供 GDS 使用。
2. 同一 family 的元件若採多 instance，command 名稱應可重用。
3. 大型資料傳輸不透過 command payload 搬運。
4. 所有命名需與 [00_dev_spec_main.md](./00_dev_spec_main.md) 及 [07_boot_update_manager.md](./07_boot_update_manager.md) 一致。

## 2. Base ID 與命名策略

### 2.1 Base ID 範圍

| Base ID 範圍 | 用途 |
|-------------|------|
| `0x10000000` - `0x1000FFFF` | F' 核心 / deployment 基礎元件 |
| `0x10020000` - `0x1002FFFF` | 平台與 HAL 類元件 |
| `0x10030000` - `0x1003FFFF` | 子系統 Bridge / 應用元件 |
| `0x10040000` - `0x1004FFFF` | CSP 與內部網路元件 |
| `0x10050000` - `0x1005FFFF` | 通訊次系統元件 |
| `0x10060000` - `0x1006FFFF` | Boot / Update 元件 |

### 2.2 多 instance 命名原則

若元件屬於可重用 family，則：

- FPP command / tlm / event 名稱保持 generic
- 由 instance name 決定實際裝置語意
- GDS 顯示時以 `instance.command` 區分，例如：
  - `sbandCtrl.RADIO_ENABLE`
  - `uhfCtrl.RADIO_ENABLE`

## 3. 指令設計

### 3.1 系統與模式控制

| 指令名稱 | 元件 | 參數 | 說明 |
|---------|------|------|------|
| `MODE_SET` | `ModeManager` | `mode: SatMode` | 設定衛星模式 |
| `MODE_GET` | `ModeManager` | — | 查詢當前模式 |
| `HEALTH_ENABLE` | `HealthMonitor` | `enable: bool` | 啟用或停用健康檢查 |
| `HEALTH_SET_THRESHOLD` | `HealthMonitor` | `item: HealthItem, value: F32` | 設定閾值 |

### 3.2 CSP 與內部網路

| 指令名稱 | 元件 | 參數 | 說明 |
|---------|------|------|------|
| `CSP_INIT` | `CspBridge` | `nodeId: U8` | 初始化 CSP node |
| `CSP_PING` | `CspBridge` | `targetNode: U8, timeoutMs: U32` | ping 遠端節點 |
| `CSP_SEND_RAW` | `CspBridge` | `targetNode: U8, targetPort: U8, data: string size 240` | 發送小型除錯封包 |

### 3.3 EPS 代理

| 指令名稱 | 元件 | 參數 | 說明 |
|---------|------|------|------|
| `EPS_GET_STATUS` | `EpsBridge` | — | 查詢完整狀態 |
| `EPS_SET_PDU` | `EpsBridge` | `channel: U8, enable: bool` | 切換配電通道 |
| `EPS_SET_HEATER` | `EpsBridge` | `heaterId: U8, enable: bool` | 控制加熱器 |
| `EPS_RESET` | `EpsBridge` | — | 重設 EPS 模擬器狀態 |

### 3.4 ADCS 代理

| 指令名稱 | 元件 | 參數 | 說明 |
|---------|------|------|------|
| `ADCS_SET_MODE` | `AdcsBridge` | `mode: AdcsMode` | 切換 ADCS 模式 |
| `ADCS_SET_TARGET` | `AdcsBridge` | `ra: F64, dec: F64` | 設定目標指向 |
| `ADCS_GET_ATTITUDE` | `AdcsBridge` | — | 查詢姿態與角速度 |
| `ADCS_CALIBRATE` | `AdcsBridge` | `sensor: AdcsSensor` | 觸發校準 |

### 3.5 外部通訊鏈路

#### 3.5.1 `CommController`

| 指令名稱 | 元件 | 參數 | 說明 |
|---------|------|------|------|
| `COMM_SET_ACTIVE` | `CommController` | `band: CommBand` | 選擇活動頻段 |
| `COMM_START_PASS` | `CommController` | `durationSec: U32` | 開始通訊窗口 |
| `COMM_STOP_PASS` | `CommController` | — | 結束通訊窗口 |

#### 3.5.2 `RadioController` family

| 指令名稱 | instance 範例 | 參數 | 說明 |
|---------|---------------|------|------|
| `RADIO_ENABLE` | `sbandCtrl` / `uhfCtrl` | `enable: bool` | 開關 radio |
| `RADIO_SET_POWER` | `sbandCtrl` / `uhfCtrl` | `powerDbm: U8` | 設定發射功率 |
| `RADIO_SET_FREQ` | `sbandCtrl` / `uhfCtrl` | `freqHz: U32` | 設定頻率 |
| `RADIO_GET_STATUS` | `sbandCtrl` / `uhfCtrl` | — | 讀取 radio 狀態 |

### 3.6 Boot & Update Manager

> 大檔映像不經 command payload 直接搬運。映像應先經 file uplink 或 staging file 寫入。

| 指令名稱 | 元件 | 參數 | 說明 |
|---------|------|------|------|
| `BOOT_STATUS` | `BootManager` | — | 查詢目前 slot 與狀態 |
| `BOOT_PREPARE_UPDATE` | `BootManager` | `imageSize: U32, digest: string size 64` | 宣告將驗證的映像資訊 |
| `BOOT_VERIFY_STAGED_IMAGE` | `BootManager` | `stagingPath: string size 128` | 驗證 staging file |
| `BOOT_ACTIVATE_STAGED_IMAGE` | `BootManager` | — | 將已驗證映像設為下次啟動目標 |
| `BOOT_CONFIRM` | `BootManager` | — | 啟動後確認當前版本正常 |
| `BOOT_ROLLBACK` | `BootManager` | — | 強制回滾到前一個已知穩定 slot |

## 4. 遙測設計

### 4.1 更新頻率

| 更新率 | 用途 |
|--------|------|
| 1 Hz | 系統基本狀態、EPS、ADCS、主要鏈路狀態 |
| 0.5 Hz | CSP 統計、模式管理、radio 狀態 |
| 0.1 Hz | 健康檢查、buffer / storage / update 管理狀態 |
| on-demand | 較重或僅除錯需要的欄位 |

### 4.2 系統遙測

| 通道名稱 | 型別 | 更新率 | 說明 |
|---------|------|--------|------|
| `SYS_MODE` | `SatMode` | 0.5 Hz | 衛星模式 |
| `SYS_UPTIME_SEC` | `U32` | 1 Hz | 運行秒數 |
| `SYS_CPU_USAGE` | `F32` | 1 Hz | CPU 使用率 |
| `SYS_MEM_RSS_MB` | `F32` | 1 Hz | 行程 RSS |
| `SYS_REBOOT_COUNT` | `U16` | 0.1 Hz | 啟動次數 |

### 4.3 CSP 遙測

| 通道名稱 | 型別 | 更新率 | 說明 |
|---------|------|--------|------|
| `CSP_TX_PACKETS` | `U32` | 0.5 Hz | 發送封包數 |
| `CSP_RX_PACKETS` | `U32` | 0.5 Hz | 接收封包數 |
| `CSP_ERROR_COUNT` | `U32` | 0.5 Hz | 錯誤計數 |
| `CSP_FREE_BUFFERS` | `U32` | 0.5 Hz | 可用 buffer |

### 4.4 EPS 遙測

| 通道名稱 | 型別 | 更新率 | 說明 |
|---------|------|--------|------|
| `EPS_VBAT` | `F32` | 1 Hz | 電池電壓 |
| `EPS_IBAT` | `F32` | 1 Hz | 電池電流 |
| `EPS_SOC` | `F32` | 1 Hz | 荷電狀態 |
| `EPS_VSOLAR` | `F32` | 1 Hz | 太陽能板電壓 |
| `EPS_ISOLAR` | `F32` | 1 Hz | 太陽能板電流 |
| `EPS_TEMP_BAT` | `F32` | 1 Hz | 電池溫度 |
| `EPS_PDU_STATUS` | `U8` | 1 Hz | 配電 bitmask |
| `EPS_POWER_OUT` | `F32` | 1 Hz | 輸出功率 |

### 4.5 ADCS 遙測

| 通道名稱 | 型別 | 更新率 | 說明 |
|---------|------|--------|------|
| `ADCS_Q0` | `F64` | 1 Hz | 四元數 q0 |
| `ADCS_Q1` | `F64` | 1 Hz | 四元數 q1 |
| `ADCS_Q2` | `F64` | 1 Hz | 四元數 q2 |
| `ADCS_Q3` | `F64` | 1 Hz | 四元數 q3 |
| `ADCS_OMEGA_X` | `F32` | 1 Hz | 角速度 X |
| `ADCS_OMEGA_Y` | `F32` | 1 Hz | 角速度 Y |
| `ADCS_OMEGA_Z` | `F32` | 1 Hz | 角速度 Z |
| `ADCS_MAG_X` | `F32` | 1 Hz | 磁場 X |
| `ADCS_MAG_Y` | `F32` | 1 Hz | 磁場 Y |
| `ADCS_MAG_Z` | `F32` | 1 Hz | 磁場 Z |
| `ADCS_MODE` | `AdcsMode` | 0.5 Hz | 控制模式 |
| `ADCS_POINTING_ERR` | `F32` | 1 Hz | 指向誤差 |

### 4.6 通訊次系統遙測

#### `UartDriver` family

| 通道名稱 | 型別 | 更新率 | 說明 |
|---------|------|--------|------|
| `UART_TX_BYTES` | `U32` | 1 Hz | 發送位元組 |
| `UART_RX_BYTES` | `U32` | 1 Hz | 接收位元組 |
| `UART_TX_ERRORS` | `U32` | 0.5 Hz | 傳送錯誤 |
| `UART_RX_ERRORS` | `U32` | 0.5 Hz | 接收錯誤 |
| `UART_CONNECTED` | `bool` | 0.5 Hz | driver 狀態 |

#### `RadioController` family

| 通道名稱 | 型別 | 更新率 | 說明 |
|---------|------|--------|------|
| `RADIO_ENABLED` | `bool` | 0.5 Hz | radio 開關狀態 |
| `RADIO_TX_POWER` | `U8` | 0.5 Hz | 發射功率 |
| `RADIO_FREQ` | `U32` | 0.5 Hz | 頻率 |
| `RADIO_TEMP` | `F32` | 1 Hz | 溫度 |
| `RADIO_RSSI` | `I16` | 1 Hz | 信號強度 |

#### `CommController`

| 通道名稱 | 型別 | 更新率 | 說明 |
|---------|------|--------|------|
| `COMM_ACTIVE_BAND` | `CommBand` | 0.5 Hz | 當前頻段 |
| `COMM_PASS_ACTIVE` | `bool` | 0.5 Hz | 通訊窗口狀態 |
| `COMM_PASS_REMAINING` | `U32` | 1 Hz | 剩餘秒數 |
| `COMM_TOTAL_PASSES` | `U32` | 0.1 Hz | 累計窗口數 |

### 4.7 Boot & Update 遙測

| 通道名稱 | 型別 | 更新率 | 說明 |
|---------|------|--------|------|
| `BOOT_ACTIVE_SLOT` | `BootSlot` | 0.1 Hz | 目前啟動 slot |
| `BOOT_PENDING_SLOT` | `BootSlot` | 0.1 Hz | 下次欲啟動 slot |
| `BOOT_CONFIRMED` | `bool` | 0.1 Hz | 是否已確認 |
| `BOOT_UPDATE_PROGRESS` | `U8` | on-demand | 驗證 / 切換進度 |
| `BOOT_LAST_ERROR` | `U32` | on-demand | 最近錯誤碼 |

## 5. 事件設計

### 5.1 等級定義

| 等級 | F' Severity | 用途 |
|------|-------------|------|
| INFO | `activity low` | 例行記錄 |
| NOTICE | `activity high` | 狀態切換與重要動作 |
| WARNING | `warning low` | 可恢復異常 |
| ALARM | `warning high` | 需人工注意的異常 |
| FATAL | `fatal` | 系統無法持續安全運作 |

### 5.2 系統事件

| 事件名稱 | 等級 | 觸發條件 |
|---------|------|---------|
| `SYS_BOOT` | NOTICE | 系統啟動 |
| `SYS_MODE_CHANGE` | NOTICE | 模式變更 |
| `SYS_LOW_MEMORY` | ALARM | 記憶體超過閾值 |
| `SYS_RESOURCE_DEGRADED` | WARNING | CPU / storage / buffer 壓力過高 |

### 5.3 EPS 事件

| 事件名稱 | 等級 | 觸發條件 |
|---------|------|---------|
| `EPS_STATUS_RECEIVED` | INFO | 成功更新狀態 |
| `EPS_LOW_BATTERY` | ALARM | SoC < 20% |
| `EPS_CRITICAL_BATTERY` | FATAL | SoC < 10% |
| `EPS_OVERTEMP` | ALARM | 溫度超標 |
| `EPS_PDU_CHANGE` | NOTICE | 配電狀態變更 |
| `EPS_COMM_ERROR` | WARNING | 通訊錯誤 |

### 5.4 ADCS 事件

| 事件名稱 | 等級 | 觸發條件 |
|---------|------|---------|
| `ADCS_MODE_CHANGE` | NOTICE | 模式切換 |
| `ADCS_DETUMBLE_COMPLETE` | NOTICE | 去翻滾完成 |
| `ADCS_POINTING_ACQUIRED` | NOTICE | 指向誤差達標 |
| `ADCS_SENSOR_FAULT` | ALARM | 感測器故障 |
| `ADCS_COMM_ERROR` | WARNING | 通訊錯誤 |

### 5.5 通訊事件

| 事件名稱 | 等級 | 觸發條件 |
|---------|------|---------|
| `COMM_BAND_SWITCH` | NOTICE | 活動頻段切換 |
| `COMM_PASS_START` | NOTICE | 通訊窗口開始 |
| `COMM_PASS_END` | NOTICE | 通訊窗口結束 |
| `UART_OPEN` | NOTICE | UART 開啟成功 |
| `UART_ERROR` | WARNING | UART 操作錯誤 |
| `RADIO_POWERED_ON` | NOTICE | radio 啟用 |
| `RADIO_POWERED_OFF` | NOTICE | radio 關閉 |
| `RADIO_OVERTEMP` | ALARM | radio 溫度超標 |

### 5.6 Boot & Update 事件

| 事件名稱 | 等級 | 觸發條件 |
|---------|------|---------|
| `BOOT_UPDATE_PREPARED` | NOTICE | 更新 metadata 已登錄 |
| `BOOT_STAGE_VERIFY_OK` | NOTICE | staging image 驗證成功 |
| `BOOT_STAGE_VERIFY_FAIL` | ALARM | staging image 驗證失敗 |
| `BOOT_SLOT_SWITCHED` | NOTICE | 下次啟動 slot 已切換 |
| `BOOT_CONFIRMED` | NOTICE | 啟動後完成確認 |
| `BOOT_ROLLBACK_TRIGGERED` | ALARM | 觸發回滾 |

## 6. 共用型別

```fpp
module OBC {
  enum SatMode {
    SAFE = 0
    NOMINAL = 1
    LOW_POWER = 2
    DEBUG = 3
    UPDATE = 4
  }

  enum AdcsMode {
    IDLE = 0
    DETUMBLE = 1
    POINTING = 2
    SLEW = 3
  }

  enum CommBand {
    SBAND = 0
    UHF = 1
  }

  enum BootSlot {
    SLOT_A = 0
    SLOT_B = 1
    NONE = 255
  }
}
```

## 7. 設計約束

1. `CSP_SEND_RAW` 僅供小型除錯封包，不得替代正式資料通道。
2. `BootManager` 不得提供 `BOOT_WRITE_CHUNK` 類型 command。
3. 多 instance 元件的字典應透過 instance name 區分，不重新命名 command family。
4. telemetry 更新頻率可新增，但不得與既有頻率規劃衝突。

## 8. 驗收準則

| 編號 | 驗收項目 | 通過條件 |
|------|---------|---------|
| 1 | 命名一致 | 與其他章節中的元件與 slot 名稱一致 |
| 2 | Boot 流程一致 | 不出現 chunk-based firmware command |
| 3 | 多 instance 一致 | `RadioController` / `UartDriver` 使用 generic naming |
| 4 | 更新頻率一致 | 與主文件與測試文件一致 |
| 5 | GDS 可用性 | command / tlm / event 設計可直接生成字典 |
