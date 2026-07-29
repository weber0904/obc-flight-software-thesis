# 測試紀錄：gps-live-uart-hardening-v1

- 日期：2026-04-27
- 層級：L1 / L2 / L3 / L4
- 環境：
  - host: macOS development machine
  - target baseline: `obc.local:/dev/serial0` live GPS path already governed by `gps-live-uart-source-v1`
- 關聯 change：`openspec/changes/archive/2026-04-27-gps-live-uart-hardening-v1/`
- 關聯 prior evidence：`docs/test-records/gps-live-uart-source-v1/README.md`

## 1. 目標

本 change 不是新增新的 GPS path，而是補強既有 `live-uart` path 的 low-level robustness：

- blank serial lines 不應被當成 `NO_SOURCE_DATA`
- impossible baudrate env values 應在 narrowing 前被安全拒絕並 fallback
- PTY-backed test setup 應 fail fast
- governed target probe 應清理 local temporary artifacts

本次不重新聲稱：

- 新的 target-side GPS hardware capability
- live-sky fix
- PPS / timing behavior
- comm / RF integration

## 2. 驗證指令

```bash
bash -n scripts/run_rpi_gps_live_probe.sh
bash scripts/run_verification_ci.sh build-artifacts/gps-live-uart-hardening-v1-local
./build-fprime-automatic-native-ut/bin/Darwin/gps_support_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_GpsBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/gps_bridge_contract_test
./build-fprime-automatic-native-ut/bin/Darwin/gps_bridge_cached_state_integration_test
bash scripts/run_gps_hosted_probe.sh
```

## 3. 觀察結果

- `gps_support_unit_test` 新增 blank-line case，證明 serial source 會略過空行後繼續回傳下一筆有效 sentence。
- `gps_support_unit_test` 也新增 blank-line noise timeout case，證明即使持續收到空白 line，單次 `nextSentence()` 呼叫仍會在 bounded total timeout 內返回，不會因為 `continue` 迴圈失去上界。
- `OBC_Components_GpsBridge_ut_exe` 新增 classic harness case，證明 overflow baudrate env 會 fallback 到 governed default，而不是讓 `LIVE_UART` activation 因 narrowing garbage value 失敗。
- `gps_bridge_contract_test` 同步驗證 overflow baudrate fallback 的 runtime contract。
- `run_rpi_gps_live_probe.sh` 經 `bash -n` 驗證後，現在會清理 `/tmp/obc-rpi-gps-live.*` probe 目錄。
- `bash scripts/run_gps_hosted_probe.sh` 仍通過，代表 fake/replay hosted baseline 未被 hardening 破壞。

## 4. 測試結果

- `bash -n scripts/run_rpi_gps_live_probe.sh`: `PASS`
- `bash scripts/run_verification_ci.sh build-artifacts/gps-live-uart-hardening-v1-local`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/gps_support_unit_test`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_GpsBridge_ut_exe`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/gps_bridge_contract_test`: `PASS`
- `./build-fprime-automatic-native-ut/bin/Darwin/gps_bridge_cached_state_integration_test`: `PASS`
- `bash scripts/run_gps_hosted_probe.sh`: `PASS`

## 5. 驗收結論

- `PASS`: live UART GPS source now ignores ignorable blank serial lines within the bounded polling path
- `PASS`: overflow baudrate env values no longer depend on narrowing behavior and safely fall back to the governed default
- `PASS`: PTY-backed test and probe hygiene are improved without changing the public GPS contract
- `PASS`: no regression observed in the hosted GPS baseline
