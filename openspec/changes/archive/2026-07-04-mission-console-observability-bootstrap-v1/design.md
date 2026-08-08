## Context

Mission Console UX uplift 已把 `/`、`/ops`、`/readback`、`/packet-lab`、
`/surfaces` 變成較可用的操作介面，但明確讀回仍有一個未收斂的
契約缺口：對 shared `update on change` surface 而言，若 `GET_*` 只是重寫 OBC
快取，而狀態值剛好沒有變，地面就可能完全看不到新的下傳證據。

目前 repo 內已經有：

- Gateway-owned listener and snapshot 快取
- 現成 shared channel 強制重送樣板，例如 `MODE_GET`、`GPS_GET_STATE`、
  `EPS_GET_STATUS`、`ADCS_GET_ATTITUDE`、`TTC_GET_STATUS`、
  `STORAGE_GET_STATUS`、`COMM_GET_STATUS`、`BOOT_STATUS`
- 現成 event/report 型讀回，例如 `GET_RECOVERY_STATUS`、
  `GET_HW_WATCHDOG_STATUS`、`PAYLOAD_GET_STATUS`、
  `PAYLOAD_GET_CAPABILITIES`、`PAYLOAD_GET_LAST_CAPTURE_METADATA`、
  `GET_PERSISTENT_FAULT_HISTORY`、`GET_RESET_CAUSE`、`GET_BOOT_COUNT`

因此本 change 的重點是明確定義：

- 哪些欄位只保留 `持續下傳`
- 哪些欄位應同時具備 `狀態變更下傳 + GET_* 強制再送一次`
- 哪些欄位只需要 `GET_* 強制再送一次`
- 哪些欄位應降到 `診斷 / 檢視`

以及哪些 `GET_*` 必須直接在既有語意值上產生新的可下傳樣本，
哪些則保留 event/report 型 bounded 結果面，但要有正式 closing 契約。

## Goals / Non-Goals

**Goals:**

- 將 Mission Console 的操作者狀態查詢收斂成正式的明確刷新 / 讀回規則
- 對 shared `update on change` telemetry-backed 主操作狀態欄位，明確規範 `GET_*` 的樣本產生行為
- 讓 dashboard、讀回頁與工程檢查面各自使用清楚分離的資料契約
- 在不破壞 current secure/operator authority boundary 的前提下，優先重用既有 `GET_*` / status command surface

**Non-Goals:**

- 不在這一輪重做所有現有 telemetry owner
- 不把所有主操作狀態欄位都改成持續即時下傳
- 不把每個 `GET_*` 都改造成新的 flight protocol
- 不取代 stock `fprime-gds`
- 不處理更大範圍的 pass planner 或 scheduler 議題

## Decisions

### 1. 明確 `GET_*` 刷新必須產生真正的新樣本

對 shared `update on change` telemetry-backed 主操作狀態欄位，
對應的 `GET_*` 路徑必須產生新的可下傳樣本。
只更新 OBC 端快取並不足夠。

### 2. 預設優先重用同一個既有 channel

若同一個語意值已經有既有 telemetry channel，預設做法不是拆成兩個面，
而是：

- 平常保留既有主操作 channel 語意
- 狀態變更才送的欄位，繼續維持狀態變更下傳
- `GET_*` 時忽略值未變抑制
- 在同一個 channel 上重送目前值

只有在既有面本來就屬於查詢結果 / 歷史回報語意時，才保留 event-style 回應。

### 3. 使用兩類明確讀回 surface：強制遙測樣本與既有回報型 event

短期內，明確刷新 / 讀回採用兩類現有 surface：

- `強制遙測樣本`
  - `MODE_GET`
  - `TTC_GET_STATUS`
  - `GPS_GET_STATE`
  - `STORAGE_GET_STATUS`
  - `ADCS_GET_ATTITUDE`
  - `EPS_GET_STATUS`
  - `COMM_GET_STATUS`
- `既有查詢結果 / 歷史回報 event`
  - `GET_RECOVERY_STATUS`
  - `GET_HW_WATCHDOG_STATUS`
  - `PAYLOAD_GET_STATUS`
  - `PAYLOAD_GET_CAPABILITIES`
  - `PAYLOAD_GET_LAST_CAPTURE_METADATA`
  - `GET_PERSISTENT_FAULT_HISTORY`
  - `BOOT_STATUS`
  - `GET_RESET_CAUSE`
  - `GET_BOOT_COUNT`

### 4. 把 `ADCS`、`EPS`、`GPS` 的狀態刷新收斂成正式樣板

`ADCS_MODE` 與 `EPS_PDU_STATUS` 都是 dashboard 相關的主操作狀態值，但目前實務上要刷新它們，仍可能需要較詳細的讀回動作
（`ADCS_GET_ATTITUDE`、`EPS_GET_STATUS`）。
本設計接受它作為第一版路徑，但前提是這些指令能替 dashboard 真正需要的狀態子集合送出強制遙測樣本。
後續仍優先考慮補更輕量的 status surface。

2026-07-01 實作收斂：

- `MODE_GET` 已完成第一個 shared 主操作狀態樣板
  - 在同一組既有 channel 上強制重送
    `SYS_MODE` / `SYS_UPTIME_SEC` / `SYS_REBOOT_COUNT`
  - 不新增第二個 mode surface
- `EPS_GET_STATUS` 已完成第一個完整 bridge 三層樣板
  - 持續下傳：
    `EPS_VBAT` / `EPS_IBAT` / `EPS_SOC` / `EPS_TEMP_BAT`
  - 狀態變更下傳 + 明確刷新強制重送：
    `EPS_PDU_STATUS` / `EPS_HEATER_ENABLED` / `EPS_OVERCURRENT_FLAGS`
  - 只在明確刷新時重送：
    `EPS_VSOLAR` / `EPS_ISOLAR` / `EPS_POWER_OUT`
- `ADCS_GET_ATTITUDE` 已完成第二個完整 bridge 三層樣板
  - 持續下傳：
    `ADCS_Q0..Q3` / `ADCS_OMEGA_X..Z`
  - 狀態變更下傳 + 明確刷新強制重送：
    `ADCS_MODE`
  - 只在明確刷新時重送：
    `ADCS_MAG_X..Z` / `ADCS_POINTING_ERR`
- `GPS_GET_STATE` 已完成第一個不需要額外 raw refresh port 的 shared channel 強制重送樣板
  - 狀態變更下傳 + 明確刷新強制重送：
    `GPS_SOURCE_MODE` / `GPS_HAVE_SAMPLE` / `GPS_FIX_VALID` / `GPS_SAT_COUNT`
  - 只在明確刷新時重送：
    `GPS_LAT_DEG` / `GPS_LON_DEG` / `GPS_ALT_M` / `GPS_SPEED_MPS` /
    `GPS_COURSE_DEG` / `GPS_HDOP` / `GPS_UTC_SEC_OF_DAY` /
    `GPS_UTC_DATE_YMD` / `GPS_ACCEPTED_SENTENCES` / `GPS_REJECTED_SENTENCES`
- `TTC_GET_STATUS` 已完成 `2+3 / 3` shared status 刷新樣板
- `STORAGE_GET_STATUS` 已完成 `2+3 / 3` shared status 刷新樣板
- `COMM_GET_STATUS` 已完成 shared comm posture 刷新樣板
- `GET_RECOVERY_STATUS`、`GET_HW_WATCHDOG_STATUS` 已完成第一個
  `單筆結果型 event readback` 樣板
- payload 三個 query command 已完成第二個 `單筆結果型 event readback`
  tranche
- `GET_PERSISTENT_FAULT_HISTORY` 已完成 `多筆結果型 event readback`
  closing
- `BOOT_STATUS`、`GET_RESET_CAUSE`、`GET_BOOT_COUNT` 已完成 boot family
  closing
- 因此本 change 的 shared channel tranche 與 event/report 型讀回 closing
  已全部收斂完成；後續若再延伸，只屬較輕量 status surface 或 UI/UX
  broadening，不再是這個 change 的未完成缺口

### 5. 必要時保留明確刷新結果的來源資訊

snapshot layer 應能區分 dashboard 上可見的狀態值是來自：

- 操作者明確刷新
- 後續由狀態變化觸發的更新

### 6. 刷新觸發只保留明確操作

刷新只應由以下明確操作觸發：

- explicit operator `GET_*`
- explicit operator refresh

### 7. `event/report 型讀回` 固定分成三類 closing 契約

對本來就不是 shared channel 強制重送問題的 query/result surface，
這一輪固定用三類來收斂：

- `單筆結果型`
  - `GET_RECOVERY_STATUS`
  - `GET_HW_WATCHDOG_STATUS`
  - `PAYLOAD_GET_STATUS`
  - `PAYLOAD_GET_CAPABILITIES`
  - `PAYLOAD_GET_LAST_CAPTURE_METADATA`
- `多筆結果型`
  - `GET_PERSISTENT_FAULT_HISTORY`
- `boot / 歷史確認型`
  - `BOOT_STATUS`
  - `GET_RESET_CAUSE`
  - `GET_BOOT_COUNT`

這三類之後不再混成 generic `event-based`。

### 8. `event/report 型讀回` 的成功關閉要明確區分 main evidence 與 fallback

固定規則如下：

- `單筆結果型`
  - happy path：`fresh single event`
  - `command completion` 只能作 timeout 保險，不可當主要成功證據
- `多筆結果型`
  - happy path：`fresh status event + fresh record event group`
  - 若只有 `status` 沒有 `record`，預設不算完整成功，除非 `status` 明確表示空集合
- `boot / 歷史確認型`
  - `event-based preferred, completion fallback allowed`
  - 但 `completion fallback` 只能代表「指令執行完成」，不能冒充「取得完整 boot 狀態」

## Risks / Trade-offs

- [明確刷新目前仍可能重用較重的 `GET_*` 指令] -> 先從小而有界的一組開始，後續再把較輕量的 status surface 列為正式追蹤項目。
- [強制遙測樣本會繞過 generated `update on change` helper] -> 把這種繞過限制在 explicit `GET_*` handler 內，不要全域削弱背景遙測行為。
- [Dashboard 的來源資訊可能變得難以判讀] -> 當明確刷新覆寫快取中的狀態值時，保留時間戳與來源資訊。
- [明確刷新與背景常駐狀態會混在一起] -> 保持讀回與刷新都是明確的操作動作，不讓 shared cached state 冒充自己的新鮮證據。
- [hosted proof 容易被 shared runtime 節奏誤設污染] -> 對 hosted manual dual-GDS path，shared OBC runtime 必須維持 current baseline `tick_ms=1000`；`tick_ms=250` 會放大背景遙測並製造 `ComCcsds.comQueue.QueueOverflow` 假故障。

## Migration Plan

1. 完成明確刷新 / 讀回需求與面向操作者欄位地圖的收斂。
2. 為選定的 `GET_*` 指令新增或重構 component-local helper，讓它們能替主操作狀態欄位送出強制遙測樣本。
3. 補上或收斂 Mission Console 的刷新處理與 snapshot provenance 欄位。
4. 更新 dashboard / 讀回頁呈現方式，讓刷新結果與其新鮮度 / 來源可被看見。
5. 更新 runbook 與 proof/evidence 文件，然後重跑 Mission Console local、hosted、target 驗證流程。

## Current Status Notes

- `MODE_GET` flight-side forcing path：已實作並驗證
- `EPS_GET_STATUS` / `EPS_SET_*` explicit refresh path：已實作並驗證
- `ADCS_GET_ATTITUDE` / `ADCS_SET_*` explicit refresh path：已實作並驗證
- `GPS_GET_STATE` shared channel forced-refresh path：已實作並驗證
- `TTC_GET_STATUS` shared channel forced-refresh path：已實作並驗證
- `STORAGE_GET_STATUS` shared channel forced-refresh path：已實作並驗證
- `COMM_GET_STATUS` shared channel forced-refresh path：已實作並驗證
- Mission Console native-log suffix search safety：已補回歸測試
- Mission Console provenance / freshness metadata：已實作並驗證
- hosted proof 目前需要額外記住一條治理前提：
  shared hosted runtime tick-rate 必須跟 current baseline 對齊，否則 queue overflow 會污染 readback 結論
- 2026-07-03 已完成第一個 `單筆結果型 event readback` 樣板：
  - `GET_RECOVERY_STATUS`
  - `GET_HW_WATCHDOG_STATUS`
  - happy path 固定要求 `fresh single event`
  - `command completion` 只作 fallback 證據，不可關閉成功
  - readback 結果已能區分 `main evidence`、`fallback evidence`、
    `incomplete result`
- 2026-07-03 已完成第二個 `單筆結果型 event readback` tranche：
  - `PAYLOAD_GET_STATUS`
  - `PAYLOAD_GET_CAPABILITIES`
  - `PAYLOAD_GET_LAST_CAPTURE_METADATA`
  - 這三個 command 目前也固定要求：
    - `fresh single event` 才能關閉成功
    - fresh event 必須可完整結構化；缺必要欄位時標成
      `incomplete result`
    - `command completion` 只能留作 fallback 證據，不可冒充成功
- 這一輪已補齊：
  - `GET_PERSISTENT_FAULT_HISTORY`
  - `BOOT_STATUS`
  - `GET_RESET_CAUSE`
  - `GET_BOOT_COUNT`
