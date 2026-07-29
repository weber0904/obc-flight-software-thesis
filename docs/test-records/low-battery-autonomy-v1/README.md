# 測試紀錄：low-battery-autonomy-v1

- 日期：2026-04-03
- 層級：L1 / L2 / L3
- 環境：
  - host: macOS development machine
  - mode: hosted simulator replay
- 關聯變更：`openspec/changes/low-battery-autonomy-v1/`
- 關聯文件：
  - `openspec/changes/low-battery-autonomy-v1/specs/mission-autonomy/spec.md`
  - `openspec/changes/low-battery-autonomy-v1/specs/scenario-driven-validation/spec.md`
  - `openspec/changes/low-battery-autonomy-v1/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是在既有 hosted OBC baseline 與 scenario replay bridge 之上，建立第一版 mission autonomy slice：

- cached EPS state-of-charge 低於現有 threshold 時，自動進入 `LOW_POWER`
- 同一個 autonomy path 對 ADCS 下達第一版 sun-safe pointing
- 這一版 sun-safe pointing 先用 `POINTING` + repository-owned fixed quaternion 表示，而不是新增 `SUN_TRACKING`

本次仍然**不**處理：

- automatic exit from `LOW_POWER`
- comm/load shedding
- deploy 後 detumble autonomy
- ground-pass-aware operation
- housekeeping archive / file downlink

## 2. 本次實作重點

- 新增 `OBC/Components/MissionExecutive/`
  - `MissionExecutive` passive component
  - `MissionExecutivePolicy`，負責第一版 low-battery policy
  - narrow runtime interfaces，讓 mission layer 可讀 cached EPS state 並下達 mode / ADCS control
- `ModeManager` 新增 runtime helper，讓 mission layer 可查詢與進入 `LOW_POWER`
- `EpsBridge` 新增 cached status accessor，避免 autonomy layer 每 cycle 額外打 transport poll
- `AdcsBridge` 新增 first-version sun-safe pointing runtime command，透過既有 `POINTING` mode 與 target quaternion 下達
- topology 新增 `missionExecutive`，排在 `epsBridge` 後、`adcsBridge` 前
- 新增：
  - `OBC_Components_MissionExecutive_ut_exe`
  - `mission_executive_policy_integration_test`

## 3. 驗證指令

### 3.1 重新產生 UT build

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/fprime-venv/bin/fprime-util generate --ut -f
```

### 3.2 建置 UT tree

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/fprime-venv/bin/fprime-util build --ut
```

### 3.3 直接執行 MissionExecutive unit tests

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/OBC_Components_MissionExecutive_ut_exe
```

### 3.4 直接執行 low-battery autonomy integration test

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/mission_executive_policy_integration_test
```

### 3.5 跑完整本機 regression

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/fprime-venv/bin/fprime-util check --all
```

## 4. 觀察結果

### 4.1 MissionExecutive policy behavior

- `MissionExecutive` 只在 cached EPS status 有效時執行 policy
- 當 `soc < 20.0` 時：
  - 進入 `LOW_POWER`
  - 對 ADCS 下第一版 sun-safe pointing command
- 這一版 policy 會 latch：
  - 不會重複進入 `LOW_POWER`
  - 不會重複下相同的 ADCS pointing command
- 這一版不做 automatic recovery，符合 scope 定義

### 4.2 First-version sun-safe pointing profile

- mode：`AdcsMode::POINTING`
- repository-owned fixed quaternion：
  - `q0 = 0.9238795325`
  - `q1 = 0.0`
  - `q2 = 0.3826834323`
  - `q3 = 0.0`
- 這是第一版 sun-safe pointing proxy，不等同真正 scenario-driven sun vector tracking

### 4.3 測試結果

- `OBC_Components_MissionExecutive_ut_exe`: `PASS`
  - `LowBatteryTriggersLowPowerAndSunPointing`
  - `PolicyLatchesAfterEntry`
  - `NominalBatteryDoesNothing`
- `mission_executive_policy_integration_test`: `PASS`
  - scenario replay 從 nominal battery 推進到 low battery
  - mission policy 成功切到 `LOW_POWER`
  - ADCS 成功進入 `POINTING`
  - ADCS pointing error 收斂到 5 度以下
- `fprime-util check --all`: `PASS`
  - 15/15 tests passed
  - 包含既有 simulator integration tests、既有 OBC component UT，以及新的 mission executive tests

## 5. 驗收結論

- `PASS`：第一版 `MissionExecutive` 已建立，且能從 cached subsystem state 做 system-level autonomy action
- `PASS`：low battery 已可自動觸發 `LOW_POWER`
- `PASS`：同一條 autonomy path 已可下達第一版 sun-safe pointing command
- `PASS`：scenario-driven replay 已可驗證 low-battery autonomy case
- `PASS`：本 change 未破壞既有 simulator replay baseline、comm baseline、或其他 OBC component UT

## 6. 邊界與限制

- 目前 `LOW_POWER` 只做 mode latch 與 ADCS pointing，不含 comm/load shedding
- 目前 sun-safe pointing 是 fixed quaternion proxy，不是 `SUN_TRACKING` mode，也不是直接吃 scenario sun vector
- 目前沒有 automatic recovery / hysteresis / re-entry policy
- `Blocked-HW`：本次仍是 hosted autonomy evidence，不代表真實 ADCS hardware 或 on-orbit power behavior 已驗證
