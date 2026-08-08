## Why

CubeSat OBC 幾乎一定需要取得定位、時間與 fix 狀態資料，但目前專案尚未提供獨立的 GPS 子系統、GPS telemetry/event 契約、或可被 housekeeping archive 重用的 GPS cached state。由於真實 GPS 硬體尚未接上 Raspberry Pi，現在正適合以 fake-first 方式先建立 parser、runtime state 與 archive integration，讓後續硬體 bring-up 能建立在既有 flight-software 契約之上。

## What Changes

- 新增獨立的 `gps-subsystem` capability，而不是把 GPS 資料路徑塞進現有 `comm-subsystem`
- 建立第一版 `GpsBridge`/GPS runtime state 與 `GPS_*` command、telemetry、event 契約
- 第一版先支援 fake/replay GPS source 與 NMEA sentence parsing，允許在沒有有效衛星訊號或沒有接線時完成 hosted 驗證
- 為後續真實 Raspberry Pi UART GPS bring-up 保留明確的 transport 邊界，但本 change 不宣稱已完成真實硬體整合
- 將 GPS cached state 納入既有 housekeeping archive snapshot，使 GPS telemetry 能像 EPS/ADCS 一樣被保存並下傳審查
- 補齊 hosted 驗證證據，明確區分 fake-first GPS path 與未來 target-side UART hardware path

## Capabilities

### New Capabilities
- `gps-subsystem`: 定義第一版 GPS 子系統的 public contract、fake-first source strategy、NMEA parsing baseline、cached runtime state 與 hosted validation scope

### Modified Capabilities
- `housekeeping-archive`: 將 GPS cached state 納入 archive snapshot，而不新增第二條 GPS transport polling 路徑
- `verification-evidence`: 為 GPS fake-first hosted validation 與 deferred Raspberry Pi UART bring-up 增加可審查的證據邊界
- `verification-path-registry`: 將第一版 hosted GPS fake/replay 驗證路徑正式登記為可重用 baseline，並明確區分它與未完成的 Raspberry Pi UART hardware path

## Impact

- Affected code:
  - `OBC/Components/` 新增 GPS component family 與 parser/runtime support
  - `OBC/Top/` snapshot provider、topology、wiring
  - `simulators/` 或 project-local fake source / replay source support
  - `OBC/Components/HousekeepingArchive/` runtime snapshot schema 與 serialization
- Affected APIs:
  - 新增 `GPS_*` command / telemetry / event families
  - housekeeping snapshot/archive binary format 將擴充 GPS 欄位
- Affected systems:
  - hosted validation flow
  - future Raspberry Pi UART GPS bring-up path
