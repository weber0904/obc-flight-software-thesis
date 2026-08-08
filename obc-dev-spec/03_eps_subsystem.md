# 03 — EPS 子系統與 `EpsBridge`

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/eps-subsystem/spec.md`](../openspec/specs/eps-subsystem/spec.md)。

## 1. 文件角色

本文件定義目前 active baseline 的 EPS simulator、`EpsBridge`、其公開契約、owner-mediated libcsp 使用方式與驗證要求。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/04_eps_simulator.md` | EPS simulator 行為、CSP service、`EpsBridge` 輪詢與 fallback |
| `archive/legacy-v0/02_command_telemetry_design.md` | `EPS_*` command / telemetry / event 命名 |

## 3. 系統角色與邊界

| 元件 | 角色 |
|---|---|
| `simulators/eps/` | CSP Node 2，模擬電力系統 |
| `EpsBridge` | OBC 端代理，將 EPS 狀態映射為 F' command / telemetry / event |
| `CspRuntimeOwner` | active `TopCcsds` 內唯一 intended libcsp runtime owner；負責 deployed request serialization |
| `CspBridge` | OBC 端 CSP operator/diagnostic facade；不是 active baseline 的 deployed runtime owner |

邊界規則：

- OBC 與 EPS 間使用 libcsp request / response。
- hosted 開發 profile 可使用 libcsp 官方 ZMQHUB-backed transport；不得回退成 repo 自家的 direct ZMQ REQ/REP 作為現行基線。
- `EpsBridge` 仍採週期輪詢為主、事件告警為輔，但 active baseline 的 scheduled poll 已改為 owner-mediated async/coalesced cache refresh，而不是在 `schedIn` 內直接 blocking round-trip。

## 4. 模擬範圍

### 4.1 第一版必含量測

| 分類 | 欄位 |
|---|---|
| 電池 | 電壓、電流、SoC、溫度 |
| 太陽能板 | 電壓、電流、日照狀態 |
| PDU | 8 路開關狀態、輸出功率、過流旗標 |

### 4.2 可簡化項目

- 詳細化學電池模型
- 真實軌道與姿態耦合照明模型
- 供電暫態與 EMI 細節

### 4.3 Hosted demo power model

目前 hosted EPS simulator 不再回固定常數，而是使用 seeded、
time-continuous 的 demo load model：

- PDU mapping 固定為 `0=OBC`、`1=ADCS`、`3=Payload`、`5=S-band`、
  `6=UHF`
- `2/4/7` 仍可切 bit，但只作為 spare channel，不貢獻任何負載
- boot 預設 `pdu_status = 0x03`，也就是只有 `OBC + ADCS` 開啟
- runtime load mode 固定為 simulator-owned `normal` / `high-draw`
- `high-draw` 是 sticky external control，不會自動跟著 payload proxy 或
  OBC mode 切換
- 所有 summary telemetry 以 monotonic time 推進，並疊加 fixed-seed
  pseudo-noise，讓 trend 圖呈現非水平線但仍可重播

## 5. CSP 服務與資料結構

### 5.1 Port 分配

| Port | 服務 |
|---|---|
| 10 | `EPS_SRV_STATUS` |
| 11 | `EPS_SRV_PDU` |
| 12 | `EPS_SRV_CONFIG` |
| 13 | `EPS_SRV_RESET` |

Ports `0..3` 保留給 libcsp 內建管理服務（例如 ping / CMP），不得作為 EPS application service port。

### 5.2 封包原則

1. 單次狀態回應應可放入單一 CSP payload。
2. 所有 request / response 必須帶 `seq` 以便對應。
3. EPS active on-wire payload 定義於 `simulators/eps/EpsCspProtocol.hpp`。
4. EPS runtime DTO 與 state semantics 定義於 `simulators/eps/EpsTypes.hpp`；不得再由 shared hosted protocol header 承擔 EPS on-wire authority。

## 6. 公開契約

### 6.1 Commands

| 指令名稱 | 說明 |
|---|---|
| `EPS_GET_STATUS` | 查詢完整狀態 |
| `EPS_SET_PDU` | 切換配電通道 |
| `EPS_SET_HEATER` | 切換加熱器 |
| `EPS_RESET` | 重設 simulator 狀態 |

### 6.2 Telemetry

- `EPS_VBAT`
- `EPS_IBAT`
- `EPS_SOC`
- `EPS_VSOLAR`
- `EPS_ISOLAR`
- `EPS_TEMP_BAT`
- `EPS_PDU_STATUS`
- `EPS_POWER_OUT`

### 6.3 Events

- `EPS_STATUS_RECEIVED`
- `EPS_LOW_BATTERY`
- `EPS_CRITICAL_BATTERY`
- `EPS_OVERTEMP`
- `EPS_PDU_CHANGE`
- `EPS_COMM_ERROR`

## 7. `EpsBridge` 行為規格

1. `schedIn` 代表 EPS scheduled poll slot，但 active baseline 會先透過 `CspRuntimeOwner` 提交 async request；若前一筆同類 poll 尚未完成，新的 tick 會 coalesce，而不是在 rate group 內同步等待。
2. 收到有效回應 completion 後更新對應 telemetry、cache 與 poll-health state。
3. 回應逾時時，需記錄 `EPS_COMM_ERROR`，並在達到門檻時上報健康降級。
4. CSP 未初始化或 simulator 無回應時，不得輸出隨機值；需保留最後一次有效狀態或使用明確 default。

## 8. 驗證與驗收

可在現況完成的驗證：

- simulator 可獨立運行
- libcsp EPS path 可由 `eps_csp_integration_test` / `scripts/run_eps_csp_integration.sh` 查詢
- `EpsBridge` telemetry 可由 GDS 觀察
- PDU 切換與低電量事件可由模擬狀態觸發
- `set-load-mode normal|high-draw` 可透過 simulator control socket 直接
  製造可見的電流與 SoC 斜率變化

不可在現況完成的驗證：

- 真實 EPS 硬體通訊
- 真實供電與熱行為量測

以上項目標記為 `Blocked-HW`。

## 9. 外部控制面

這一步刻意不新增新的 `EPS_*` public command。demo 與 probe 使用的外部
控制仍是 simulator Unix socket，而不是 flight software contract：

- `set-soc <value> [transition-sec]`
- `set-load-mode normal`
- `set-load-mode high-draw`
- `drop-status <count>`
