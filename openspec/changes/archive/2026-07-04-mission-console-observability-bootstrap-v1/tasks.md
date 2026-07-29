## 1. 明確刷新契約與來源資訊

- [x] 1.1 為操作者觸發的 `GET_*` 與刷新動作補上 Mission Console 的明確刷新 / 讀回流程編排。
- [x] 1.2 擴充 snapshot 或 dashboard cache state，讓主操作狀態欄位在明確刷新覆寫快取值時，能記錄 `refresh` 或 `live-update` 等時間戳與來源資訊。
- [x] 1.3 依最新分類表確認哪些欄位屬 `1`、`2+3`、`3`、`4`，避免 dashboard、readback、diagnostics 再混用同一套模糊規則。

## 2. Flight 樣本產生與明確刷新

- [x] 2.0 為 `MODE_GET` 完成第一個 shared 主操作狀態強制遙測樣本樣板，驗證不切換 mode 時仍會在同一組 channel 上重送 `SYS_MODE`、`SYS_UPTIME_SEC`、`SYS_REBOOT_COUNT`。
- [x] 2.1 為 `MODE_GET`、`TTC_GET_STATUS`、`GPS_GET_STATE`、`STORAGE_GET_STATUS` 這類 shared 主操作狀態讀回動作，實作在既有 channel 上的強制遙測樣本送出。
- [x] 2.1a 完成 `GpsBridge` 的 shared 主操作狀態刷新樣板：
  `GPS_GET_STATE` 現在會在同一組既有 channel 上重送
  `GPS_SOURCE_MODE`、`GPS_HAVE_SAMPLE`、`GPS_FIX_VALID`、`GPS_SAT_COUNT`
  與詳細 GPS 狀態，不再只更新 flight-side 快取。
- [x] 2.2 重新評估並記錄哪些既有 event-based status/report surface 仍可接受作為有界查詢結果，包括 recovery、watchdog、boot、payload status 與 persistent-fault history。
- [x] 2.2a 先完成第一個 `單筆結果型 event readback` 樣板：
  `GET_RECOVERY_STATUS`、`GET_HW_WATCHDOG_STATUS`
  現在都要求以 `fresh single event` 作主要成功證據；`command completion`
  只能作 timeout 保險，不可冒充完整 query result，且結果需能區分
  `main evidence`、`fallback evidence`、`incomplete result`。
- [x] 2.2a-1 收斂 payload `單筆結果型 event readback`：
  `PAYLOAD_GET_STATUS`、`PAYLOAD_GET_CAPABILITIES`、
  `PAYLOAD_GET_LAST_CAPTURE_METADATA` 現在也固定要求以完整可結構化的
  `fresh single event` 作主要成功證據；若 fresh event 缺必要欄位，必須標成
  `incomplete result`，不可只因為 event 名稱命中就關閉成功。
- [x] 2.2b 收斂 `多筆結果型 event readback` 契約：
  `GET_PERSISTENT_FAULT_HISTORY` 現在固定要求 `fresh status event + fresh record event group`
  才算完整成功；若只有 `status` 沒有 `record`，除非明確表示空集合，否則不得關閉成功。
- [x] 2.2c 收斂 `boot / 歷史確認型 readback` 契約：
  `BOOT_STATUS` 現在固定為 `shared boot telemetry forced-resend`；
  `GET_RESET_CAUSE`、`GET_BOOT_COUNT` 固定要求 `fresh single event`，
  `command completion` 只能表示指令執行完成，不可冒充完整 boot truth。
- [x] 2.3 決定並實作 `ADCS` 與 `EPS` 的短期刷新路徑，包含是否暫時重用 `ADCS_GET_ATTITUDE` 與 `EPS_GET_STATUS` 來提供明確讀回所需的新鮮證據。
- [x] 2.3a 完成 `EpsBridge` 短期刷新路徑：
  `EPS_GET_STATUS` 與成功的 `EPS_SET_PDU` / `EPS_SET_HEATER` / `EPS_RESET`
  已走 explicit refresh helper；`EPS_VBAT` / `EPS_IBAT` / `EPS_SOC` /
  `EPS_TEMP_BAT`、`EPS_PDU_STATUS`、`EPS_HEATER_ENABLED`、
  `EPS_OVERCURRENT_FLAGS`、`EPS_VSOLAR` / `EPS_ISOLAR` / `EPS_POWER_OUT`
  的 operator-facing 分類已固定。
- [x] 2.3b 收斂 `ADCS` 短期刷新路徑：
  `ADCS_GET_ATTITUDE` 與成功的 `ADCS_SET_MODE` / `ADCS_SET_TARGET` /
  `ADCS_CALIBRATE` / `RESET` 已走 explicit refresh helper；`ADCS_Q0..Q3`、
  `ADCS_OMEGA_X..Z`、`ADCS_MODE`、`ADCS_MAG_X..Z`、`ADCS_POINTING_ERR`
  的 operator-facing 分類已固定。
- [x] 2.4 確保明確刷新會更新 dashboard 快取中的狀態值，但不把 console 變成隱藏式連續輪詢。
- [x] 2.5 為 `CommController` 補出第一版 `COMM_GET_STATUS` 正式路徑，讓
  `COMM_ACTIVE_BAND`、`COMM_PRIMARY_COMMAND_LINK`、`COMM_PRIMARY_TELEMETRY_LINK`、
  `COMM_PRIMARY_FILE_LINK`、`COMM_S_BAND_AVAILABLE`、`COMM_UHF_AVAILABLE`、
  `COMM_S_BAND_AVAILABILITY_REASON`、`COMM_UHF_AVAILABILITY_REASON`、
  `COMM_FDIR_FAULT_LATCHED`、`COMM_FDIR_FAULT_KIND`
  在值未變時仍可由操作者主動查詢重送；這一輪明確不把
  `COMM_S_BAND_LIVE_OBSERVABILITY_ACTIVE`、`CommEgressMux` packet counters、
  `GroundLinkDriver` 低階 transport 計數拉進同一批。

## 3. Dashboard 與操作者介面

- [x] 3.1 更新 dashboard / 讀回頁呈現，讓明確刷新結果可見，並能在操作者真的要求該資料時取代過時的 `Unavailable` 狀態顯示。
- [x] 3.2 加入操作者可見的新鮮度或來源提示，讓使用者分得出顯示值是來自明確刷新還是後續狀態變化更新。

## 4. 驗證與文件

- [x] 4.1 擴充 Mission Console 自動化測試，涵蓋明確刷新觸發條件、快取更新行為與 provenance metadata。
- [x] 4.1a 為 native log byte-offset slicing 問題補回歸測試，避免 manual Mission Console readback 因字串切片錯位而漏抓 fresh channel。
- [x] 4.2 為指定 `GET_*` 路徑補上聚焦測試，驗證強制遙測樣本行為，以及在明確刷新期間相同值仍會重新下傳。
- [x] 4.2a 為 `MODE_GET` 補上聚焦驗證，確認在不做 `MODE_SET` 的情況下，相同值仍會重新下傳並被 Mission Console readback 撈回。
- [x] 4.2b 為 `EPS_GET_STATUS` / `EPS_SET_*` 補上聚焦驗證，確認相同值情境下
  explicit refresh 仍會重送 operator-facing EPS 狀態，且 Mission Console
  readback 以新的 EPS channel 樣本作為主要成功證據。
- [x] 4.2c 為 `ADCS_GET_ATTITUDE` / `ADCS_SET_*` 補上聚焦驗證，確認相同值
  情境下 explicit refresh 仍會重送完整 ADCS operator-facing refresh 組，
  且 Mission Console readback 以 `ADCS_MODE` / `ADCS_Q0` /
  `ADCS_OMEGA_X` 作為主要成功證據。
- [x] 4.2d 為 `GPS_GET_STATE` 補上聚焦驗證，確認在不切換 source mode、
  fix 狀態不變的情況下，相同值仍會重新下傳並被 Mission Console
  readback 以 `GPS_SOURCE_MODE` / `GPS_FIX_VALID` / `GPS_LAT_DEG`
  關閉成功。
- [x] 4.2e 為 `COMM_GET_STATUS` 補上聚焦驗證，確認在 route / availability /
  FDIR posture 值未變的情況下，相同值仍會重新下傳並被 Mission Console
  readback 以 `COMM_ACTIVE_BAND` / `COMM_S_BAND_AVAILABLE` /
  `COMM_UHF_AVAILABLE` / `COMM_FDIR_FAULT_LATCHED` 關閉成功。
- [x] 4.2f 為 payload `單筆結果型 event readback` 補上聚焦驗證，確認
  `PAYLOAD_GET_STATUS`、`PAYLOAD_GET_CAPABILITIES`、
  `PAYLOAD_GET_LAST_CAPTURE_METADATA` 都必須由完整的 `fresh single event`
  關閉成功；若只有 `command completion` 或 fresh event payload 缺欄位，
  readback 不得誤判成功。
- [x] 4.3 更新 observability roadmap/runbook/evidence 文件，說明明確刷新規則，並重跑所需的 Mission Console local、hosted、target 驗證流程。
- [x] 4.3a 更新 observability roadmap / handoff / evidence 文件，補入
  `MODE_GET`、`EPS_GET_STATUS`、`ADCS_GET_ATTITUDE`、`GPS_GET_STATE`
  與 hosted tick-rate `QueueOverflow` 收斂結論。
- [x] 4.3a-1 補上 snapshot provenance 收斂：
  operator-facing channel 現在會記錄 `live-update` / `refresh`、
  gateway 觀測時間，以及觸發該次刷新用的 command 名稱。
- [x] 4.3b 重跑目前這輪必需的聚焦驗證：
  `scripts/test_mission_console_phase1.py`、
  `OBC_Components_BootManager_ut_exe`、
  `OBC_Components_PersistentFaultManager_ut_exe`、
  `OBC_Components_GpsBridge_ut_exe`、
  `OBC_Components_EpsBridge_ut_exe`、
  `OBC_Components_AdcsBridge_ut_exe`、
  `OBC_Components_TtcPassManager_ut_exe`、
  `OBC_Components_StorageHealthBridge_ut_exe`、
  `scripts/run_mission_console_phase1_hosted_probe.sh`。
- [x] 4.3c 補齊 authoritative fresh local gate 與 target branch-head rerun：
  `bash scripts/run_verification_ci.sh build-artifacts/verification-ci-local`
  已在本輪 branch-head 內容上重新 PASS；target 端也已先確認 installed
  release provenance，再依序完成
  `scripts/ensure_target_comm_lab_baseline.sh`、
  `scripts/ensure_ground_dual_gds_baseline.sh`、
  `scripts/run_mission_console_phase1_target_probe.sh`，
  其中 `BOOT_STATUS` 以 `channel-refresh-based` 關閉、
  `GET_RESET_CAUSE` / `GET_BOOT_COUNT` 以 `fresh single event` 關閉、
  `GET_PERSISTENT_FAULT_HISTORY` 以 `fresh event group` 或合法空集合關閉。
