# 01 — 核心系統契約

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/core-system-contracts/spec.md`](../openspec/specs/core-system-contracts/spec.md)。

## 1. 文件角色

本文件定義跨模組共享的型別、命名、Base ID 與核心元件公開契約。所有 subsystem-specific command / telemetry / event 必須在各自 capability 中定義，不得回到本文件重複列管。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/02_command_telemetry_design.md` | Base ID、共用型別、核心命名與核心 command / telemetry / event |
| `archive/legacy-v0/00_dev_spec_main.md` | 一致性規則、instance naming 原則 |

## 3. 共享識別與命名策略

### 3.1 Base ID 範圍

| Base ID 範圍 | 用途 |
|---|---|
| `0x10000000` - `0x1000FFFF` | F' 核心 / deployment 基礎元件 |
| `0x10020000` - `0x1002FFFF` | 平台與 HAL 類元件 |
| `0x10030000` - `0x1003FFFF` | 子系統 bridge / 應用元件 |
| `0x10040000` - `0x1004FFFF` | CSP 與內部網路元件 |
| `0x10050000` - `0x1005FFFF` | 通訊次系統元件 |
| `0x10060000` - `0x1006FFFF` | Boot / Update 元件 |

### 3.2 Node 與 instance 命名

正式保留的第一版 node ID 如下：

- OBC = `1`
- EPS = `2`
- ADCS = `3`

命名原則如下：

1. F' 元件型別使用 PascalCase，例如 `CspBridge`、`EpsBridge`、`BootManager`。
2. 可重用 family 使用 generic contract naming，例如 `UartDriver`、`RadioController`。
3. 多 instance 以 instance name 區分實體語意，例如 `sbandCtrl.RADIO_ENABLE`、`uhfCtrl.RADIO_ENABLE`。

## 4. 共用型別

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

## 5. 核心元件職責

| 元件 | 職責 |
|---|---|
| `ModeManager` | 管理衛星模式切換與模式查詢 |
| `HealthMonitor` | 管理健康監控啟閉與閾值設定 |
| `CspBridge` | 管理 libcsp runtime 初始化、interface binding、節點存活測試、小型除錯封包與 runtime counters |

## 6. 核心公開指令契約

| 指令名稱 | owner | 參數 | 說明 |
|---|---|---|---|
| `MODE_SET` | `ModeManager` | `mode: SatMode` | 設定衛星模式 |
| `MODE_GET` | `ModeManager` | — | 查詢當前模式 |
| `HEALTH_ENABLE` | `HealthMonitor` | `enable: bool` | 啟用或停用健康監控 |
| `HEALTH_SET_THRESHOLD` | `HealthMonitor` | `item: HealthItem, value: F32` | 設定監控閾值 |
| `CSP_INIT` | `CspBridge` | `nodeId: U8` | 初始化本地 CSP node |
| `CSP_PING` | `CspBridge` | `targetNode: U8, timeoutMs: U32` | 測試遠端節點可達性 |
| `CSP_SEND_RAW` | `CspBridge` | `targetNode: U8, targetPort: U8, data: string size 240` | 發送小型除錯封包 |

規則：

- `CSP_SEND_RAW` 僅供小型除錯與 bring-up，不得替代正式資料通道。
- `MODE_*`、`HEALTH_*`、`CSP_*` 是核心 capability 的唯一公開指令族。
- `CspBridge` 擁有 internal libcsp runtime foundation，但不擁有 EPS / ADCS 的業務請求邏輯。

## 7. 核心 telemetry 與 event 契約

### 7.1 核心 telemetry

| 通道名稱 | owner | 型別 | 更新率 |
|---|---|---|---|
| `SYS_MODE` | `ModeManager` | `SatMode` | 0.5 Hz |
| `SYS_UPTIME_SEC` | system | `U32` | 1 Hz |
| `SYS_CPU_USAGE` | system | `F32` | 1 Hz |
| `SYS_MEM_RSS_MB` | system | `F32` | 1 Hz |
| `SYS_REBOOT_COUNT` | system | `U16` | 0.1 Hz |
| `CSP_TX_PACKETS` | `CspBridge` | `U32` | 0.5 Hz |
| `CSP_RX_PACKETS` | `CspBridge` | `U32` | 0.5 Hz |
| `CSP_ERROR_COUNT` | `CspBridge` | `U32` | 0.5 Hz |
| `CSP_FREE_BUFFERS` | `CspBridge` | `U32` | 0.5 Hz |

補充：

- 以上 `CSP_*` telemetry 在 foundation recovery 後必須來自 real libcsp runtime state，而不是 fake counters。
- `CSP_PING` / `CSP_SEND_RAW` 驗證的是 internal CSP foundation 診斷能力，不等於 EPS / ADCS business traffic 已遷移完成。

### 7.2 核心 events

| 事件名稱 | owner | 等級 | 說明 |
|---|---|---|---|
| `SYS_BOOT` | system | NOTICE | 系統啟動 |
| `SYS_MODE_CHANGE` | `ModeManager` | NOTICE | 模式切換 |
| `SYS_LOW_MEMORY` | system | ALARM | 記憶體壓力過高 |
| `SYS_RESOURCE_DEGRADED` | system | WARNING | CPU / storage / buffer 壓力過高 |

## 8. Ownership 邊界

| capability | 擁有的公開契約 |
|---|---|
| `core-system-contracts` | `MODE_*`、`HEALTH_*`、`CSP_*`、`SYS_*`、`CSP_*`（核心 telemetry） |
| `eps-subsystem` | `EPS_*` command / telemetry / event |
| `adcs-subsystem` | `ADCS_*` command / telemetry / event |
| `comm-subsystem` | `COMM_*`、`RADIO_*`、`UART_*` command / telemetry / event |
| `boot-update` | `BOOT_*` command / telemetry / event |

規則：

1. subsystem-specific 契約只在 owner capability 中定義。
2. 不得同時在中央文件與子系統文件維護同一組命名。
3. 若跨 capability 契約需要變更，必須在 OpenSpec change 中同時更新相關 capability。

## 9. 設計約束

1. F' 字典命名需可直接供 GDS 使用。
2. 多 instance family 保持 generic command / telemetry naming。
3. 核心 capability 只定義共享契約，不吸收子系統細節。
4. 所有新增共享型別或命名空間變更都必須先經 OpenSpec 變更。
