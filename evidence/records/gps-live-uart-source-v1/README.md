# 測試紀錄：gps-live-uart-source-v1

- 日期：2026-04-27
- 層級：L1 / L2 / L3 / L4
- 環境：
  - host: macOS development machine
  - target: `obc.local` (`Raspberry Pi 3B+`, Raspberry Pi OS Bookworm)
  - GPS module: `GY-GPS6MV2`
  - GPS wiring: direct UART to `obc.local:/dev/serial0`, `3.3V` power, shared ground
- 關聯 change：`openspec/changes/archive/2026-04-27-gps-live-uart-source-v1/`
- 關聯架構決策：`system-communication-architecture-realignment-v1`

## 1. 目標

本 change 的目標是建立第一條正式的 target-side live GPS path：

- `GY-GPS6MV2 -> obc.local:/dev/serial0 -> GpsBridge`

本次要證明的是：

- `GpsBridge` 能以 `live-uart` source mode 從真實 UART 裝置讀到 NMEA sentence
- live sentence 會更新 cached GPS state
- `acceptedSentenceCount` 會在 target runtime 中前進
- 既有 fake / replay baseline 沒被破壞

本次不證明：

- live-sky valid fix
- PPS / timing discipline
- RF / comm integration
- navigation-grade accuracy

## 2. 重要治理結論

- `obc.local:/dev/serial0` 現在是 active GPS UART path。
- 舊的 OBC-side external comm `/dev/serial0` evidence 保留為歷史證據，但不再是 current baseline。
- active comm development 暫時回到 TCP/dev paths，直到後續 comm migration slice 完成。

## 3. 驗證指令

### 3.1 本機 GPS 測試

```bash
./build-fprime-automatic-native-ut/bin/Darwin/gps_support_unit_test
./build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_GpsBridge_ut_exe
./build-fprime-automatic-native-ut/bin/Darwin/gps_bridge_contract_test
./build-fprime-automatic-native-ut/bin/Darwin/gps_bridge_cached_state_integration_test
```

### 3.2 Hosted replay regression

```bash
bash scripts/run_gps_hosted_probe.sh
```

### 3.3 Target rebuild

```bash
bash scripts/bootstrap_rpi_workspace.sh
```

### 3.4 Supplementary direct serial sample

這不是 acceptance probe，但用來先確認 `obc.local:/dev/serial0` 上真的有 GPS sentence。

```bash
ssh operator@obc.local "/bin/bash -lc 'timeout 8s sh -c '\''stty -F /dev/serial0 9600 raw -echo -echoe -echok -ixon -ixoff -icrnl -inlcr -onlcr; dd if=/dev/serial0 bs=1 count=128 status=none | od -An -tx1'\'' || true'"
```

### 3.5 Governed target probe

```bash
bash scripts/run_rpi_gps_live_probe.sh
```

## 4. 觀察結果

### 4.1 Direct serial sample

Supplementary sample 直接從 `obc.local:/dev/serial0` 讀到真實 NMEA bytes，例如：

```text
24 47 50 52 4d 43 2c 2c 56 ...
24 47 50 47 47 41 2c 2c 2c ...
24 47 50 47 53 41 2c 41 2c 31 ...
```

這代表：

- UART 裝置沒有被 login console 佔用
- GPS module 確實在輸出 sentence
- 當下是「有句子、無 fix」狀態，而不是完全沒有資料

### 4.2 Governed live-UART probe

Probe 在 `obc.local` 上觀察到：

```text
OBC runtime started. Type 'help' for commands.
EVENT: ... GPS_STATE_UPDATED : GPS state updated fix=0 satellites=0
gps source=LIVE_UART sample=yes fixValid=no ... accepted=1 rejected=0
gps get success=yes
gps source=LIVE_UART sample=yes fixValid=no ... accepted=2 rejected=0
```

關鍵事實：

- `gps source=LIVE_UART`
- `gps get success=yes`
- `sample=yes`
- `accepted` 從 `1` 前進到 `5`
- `fixValid=no`

這表示：

- `GpsBridge` 已經透過 live UART path 接收到真實 sentence
- sentence 雖然是 no-fix，但仍被正確視為有效 sample
- cached GPS state 與 accepted counter 都有前進

## 5. 測試結果

- `gps_support_unit_test`: `PASS`
- `OBC_Components_GpsBridge_ut_exe`: `PASS`
- `gps_bridge_contract_test`: `PASS`
- `gps_bridge_cached_state_integration_test`: `PASS`
- `bash scripts/run_gps_hosted_probe.sh`: `PASS`
- `bash scripts/bootstrap_rpi_workspace.sh`: `PASS`
- `bash scripts/run_rpi_gps_live_probe.sh`: `PASS`

## 6. 驗收結論

- `PASS`: first governed target-side live GPS UART path is proven
- `PASS`: `GY-GPS6MV2 -> obc.local:/dev/serial0 -> GpsBridge` works with real sentence ingress
- `PASS`: no-fix live sentences still update cached state coherently
- `PASS`: old fake / replay GPS validation path remains intact

## 7. 邊界

- 這次 evidence 不宣稱 live-sky fix 成功
- 不宣稱 PPS / precise timing
- 不宣稱 comm / RF integration
- 不宣稱 future CAN/UART subsystem migration
- 舊的 OBC-side `/dev/serial0` comm records 現在只作為 historical evidence 使用
