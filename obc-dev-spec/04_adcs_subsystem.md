# 04 — ADCS 子系統與 `AdcsBridge`

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/adcs-subsystem/spec.md`](../openspec/specs/adcs-subsystem/spec.md)。

## 1. 文件角色

本文件定義目前 active baseline 的 ADCS simulator、`AdcsBridge`、控制模式、公開契約、owner-mediated libcsp 使用方式與驗收門檻。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/05_adcs_simulator.md` | ADCS dynamics、sensor / actuator、CSP service、`AdcsBridge` 行為 |
| `archive/legacy-v0/02_command_telemetry_design.md` | `ADCS_*` command / telemetry / event 命名 |

## 3. 系統角色與邊界

| 元件 | 角色 |
|---|---|
| `simulators/adcs/` | CSP Node 3，模擬姿態動力學與控制迴路 |
| `AdcsBridge` | OBC 端代理，將 ADCS 狀態映射為 F' command / telemetry / event |
| `CspRuntimeOwner` | active `TopCcsds` 內唯一 intended libcsp runtime owner；負責 deployed request serialization |
| `CspBridge` | OBC 端 CSP operator/diagnostic facade；不是 active baseline 的 deployed runtime owner |

第一版邊界：

- transport 固定為 libcsp；hosted profile 使用 repo-managed ZMQHUB-backed CSP substrate
- ADCS simulator 作為 CSP node `3`，OBC/client node `1` 透過 ADCS-owned CSP application services 存取
- `AdcsBridge` 仍以週期輪詢為主，必要時可接受 on-demand 查詢，但 active baseline 的 scheduled poll 已改為 owner-mediated async/coalesced cache refresh，而不是在 `schedIn` 內直接 blocking round-trip

## 4. 模擬範圍

### 4.1 第一版必含能力

| 分類 | 內容 |
|---|---|
| 姿態狀態 | 四元數、角速度 |
| 感測器 | 磁力計、陀螺儀、太陽感測器（可簡化） |
| 致動器 | 反應輪、磁力矩器（模型化） |
| 控制模式 | `IDLE`、`DETUMBLE`、`POINTING` |

### 4.2 可簡化項目

- IGRF 高保真模型
- 真實星敏感測器
- 完整軌道動力學耦合
- 高階容錯控制器

### 4.3 Hosted mode dynamics

目前 hosted ADCS simulator 使用 seeded、time-continuous 的 demo dynamics，
目的不是高保真軌道幾何，而是提供可展示、可重播、非水平線的 telemetry：

- `IDLE` 保持接近 baseline，但四元數與角速度有極小 deterministic jitter
- `DETUMBLE` 以時間驅動的一階衰減逐步收斂角速度，而不是每次 request
  固定乘係數
- `POINTING` 使用固定 `60 s` synthetic pass profile，做 roll/pitch/yaw
  掃掠，再由 current attitude 一階追蹤 desired attitude
- 外部 `ADCS_SET_TARGET` 仍保留作為 pointing-error evaluation target，但
  hosted v1 的姿態曲線 owner 仍是 synthetic pass profile

## 5. CSP 服務與資料結構

| Port | 服務 |
|---|---|
| 20 | `ADCS_SRV_STATE` |
| 21 | `ADCS_SRV_MODE` |
| 22 | `ADCS_SRV_TARGET` |
| 23 | `ADCS_SRV_CALIBRATE` |

封包原則：

1. 回應需攜帶 mode、四元數、角速度與主要感測器值。
2. on-wire request / reply envelope 由 ADCS-owned CSP payload definitions 定義於 `simulators/adcs/AdcsCspProtocol.hpp`。
3. ADCS runtime DTO 與 state semantics 定義於 `simulators/adcs/AdcsTypes.hpp`；不得再由 shared hosted protocol header 承擔 ADCS on-wire authority。
4. application ports 必須避開 libcsp reserved service ports `0..3`，並維持版本控管。

## 6. 公開契約

### 6.1 Commands

| 指令名稱 | 說明 |
|---|---|
| `ADCS_SET_MODE` | 切換控制模式 |
| `ADCS_SET_TARGET` | 更新目標指向 |
| `ADCS_GET_ATTITUDE` | 查詢姿態 |
| `ADCS_CALIBRATE` | 執行對應感測器校準 |

### 6.2 Telemetry

- `ADCS_Q0`
- `ADCS_Q1`
- `ADCS_Q2`
- `ADCS_Q3`
- `ADCS_OMEGA_X`
- `ADCS_OMEGA_Y`
- `ADCS_OMEGA_Z`
- `ADCS_MAG_X`
- `ADCS_MAG_Y`
- `ADCS_MAG_Z`
- `ADCS_MODE`
- `ADCS_POINTING_ERR`

### 6.3 Events

- `ADCS_MODE_CHANGE`
- `ADCS_DETUMBLE_COMPLETE`
- `ADCS_POINTING_ACQUIRED`
- `ADCS_SENSOR_FAULT`
- `ADCS_COMM_ERROR`

## 7. `AdcsBridge` 行為規格

1. `schedIn` 代表 ADCS scheduled poll slot，但 active baseline 會先透過 `CspRuntimeOwner` 提交 async request；若前一筆同類 poll 尚未完成，新的 tick 會 coalesce，而不是在 rate group 內同步等待。
2. 有效 completion 到達後更新對應 telemetry、cache 與 poll-health。
3. 無回應時需觸發 `ADCS_COMM_ERROR` 或等價診斷事件。
4. 無效回應不得覆寫為隨機數值；需保留最後一次有效狀態並標記資料降級。
5. 感測器欄位失效時需觸發 `ADCS_SENSOR_FAULT`。

## 8. 驗收門檻

ADCS 驗收門檻寫成可配置 mission constants，第一版預設如下：

- detumble 收斂門檻：角速度範數 `< 0.05 rad/s`
- pointing 達標門檻：`ADCS_POINTING_ERR < 5 deg`

上述門檻可於後續 change 依 mission 需求調整，但調整前必須先更新正式主規格。

## 9. 驗證與驗收

可在現況完成的驗證：

- 四元數範數穩定
- 去翻滾收斂
- 指向曲線與指向誤差隨時間變化，而不是固定值
- hosted internal libcsp ADCS node `3` service path
- `AdcsBridge` telemetry 在 GDS 中可見

不可在現況完成的驗證：

- 真實 ADCS 硬體閉迴路
- 真實感測器標定

以上項目標記為 `Blocked-HW`。

## 10. 外部控制面

這一步刻意不新增新的 `ADCS_*` public command。hosted demo 與 probe 使用的
外部控制仍是 simulator Unix socket：

- `restart-pointing-pass`
- `drop-state <count>`

`TtcPassManager`、`ModeSafetyController` 與 `AdcsBridge` 的 flight software
邊界在這一步不變，只是 downstream 測試與展示不應再假設數值永遠固定。
