# 測試紀錄：deploy-detumbling-autonomy-v1

- 日期：2026-04-03
- 層級：L1 / L2 / L3
- 環境：
  - host: macOS development machine
  - mode: hosted simulator replay
- 關聯變更：`openspec/changes/deploy-detumbling-autonomy-v1/`
- 關聯文件：
  - `openspec/changes/deploy-detumbling-autonomy-v1/specs/mission-autonomy/spec.md`
  - `openspec/changes/deploy-detumbling-autonomy-v1/specs/scenario-driven-validation/spec.md`
  - `openspec/changes/deploy-detumbling-autonomy-v1/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是在既有 `MissionExecutive` 與 scenario-seeded ADCS initial-rate baseline 上，建立第一版 deployment-style detumbling autonomy slice：

- cached ADCS angular-rate norm 高於既有 detumble threshold 時，自動命令 `DETUMBLE`
- 維持 low-battery autonomy slice 不回退
- 釐清 detumbling 與 low-battery sun-safe pointing 的優先順序

本次仍然**不**處理：

- deploy phase enum / mission phase state machine
- detumble 後自動切換至下一個 phase
- 真實 hardware-in-the-loop 或 Pi-only probe
- ground-pass-aware operation
- housekeeping archive / downlink

## 2. 本次實作重點

- `MissionExecutiveRuntime` 新增：
  - `IAdcsAutonomyStatus`
  - `IAdcsDetumbleControl`
- `AdcsBridge` 新增：
  - cached ADCS state accessor
  - first-version runtime `DETUMBLE` command helper
- `MissionExecutivePolicy` 新增：
  - high-angular-rate detection
  - detumble-active state
  - detumble command issue path
  - policy ordering：高角速度 detumbling 優先於 low-battery sun-safe pointing
- `MissionExecutive` 新增 telemetry / event：
  - `MISSION_LAST_ANGULAR_RATE_NORM`
  - `MISSION_HIGH_ANGULAR_RATE_CONDITION`
  - `MISSION_DETUMBLE_ACTIVE`
  - `MISSION_DETUMBLE_COMMAND`
- rate group 順序調整為：
  - `epsBridge`
  - `adcsBridge`
  - `missionExecutive`
  讓 mission layer 可看見同一 cycle 內更新後的 cached ADCS state
- 新增 repository-owned scenario example：
  - `simulators/scenario/examples/deploy_detumble_replay_v1.csv`

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

### 3.4 直接執行 mission autonomy integration test

```bash
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/mission_executive_policy_integration_test
```

### 3.5 跑完整本機 regression

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/fprime-venv/bin/fprime-util check --all
```

## 4. 觀察結果

### 4.1 Detumbling autonomy behavior

- 當 cached ADCS angular-rate norm `> 0.05 rad/s`：
  - `MissionExecutive` 會辨識出 high-rate condition
  - 會對 ADCS 下達 `DETUMBLE`
  - mission layer telemetry 會標示 `MISSION_DETUMBLE_ACTIVE=true`
- 當 rate norm 收斂到 threshold 以下：
  - high-rate condition 會清除
  - `MISSION_DETUMBLE_ACTIVE` 會回到 `false`
- 既有 `AdcsBridge` 仍保留 subsystem-owned `ADCS_DETUMBLE_COMPLETE` 判讀與事件

### 4.2 Policy ordering with low-battery slice

- 若同時遇到：
  - low battery
  - high angular rate
- 這一版 policy 會：
  - 先進入 `LOW_POWER`
  - 但暫時**不**下 sun-safe pointing
  - 優先做 detumbling
- 這避免了 `DETUMBLE` 與 `POINTING` 在同一時間互相衝突

### 4.3 測試結果

- `OBC_Components_MissionExecutive_ut_exe`: `PASS`
  - `HighAngularRateTriggersDetumble`
  - `HighRateSuppressesSunSafePointingAtLowBattery`
  - `LowBatteryTriggersLowPowerAndSunPointing`
  - `PolicyLatchesAfterEntry`
  - `NominalBatteryDoesNothing`
- `mission_executive_policy_integration_test`: `PASS`
  - low-battery case 仍可驅動 `LOW_POWER + sun-safe pointing`
  - detumble case 會因 scenario-seeded initial rates 命令 `DETUMBLE`
  - ADCS rate norm 會收斂到 detumble threshold 以下
- `fprime-util check --all`: `PASS`
  - 15/15 tests passed

## 5. 驗收結論

- `PASS`：第一版 deployment-style detumbling autonomy 已建立
- `PASS`：scenario-seeded initial angular rates 已可驅動 mission-level `DETUMBLE` response
- `PASS`：high-rate detumbling 與 low-battery pointing 的優先順序已明確化
- `PASS`：既有 low-battery autonomy slice 未被這次 detumbling 擴充破壞
- `PASS`：本 change 未破壞既有 simulator integration baseline 與 OBC component UT baseline

## 6. 邊界與限制

- 目前 detumbling trigger 直接來自 cached ADCS rate norm，不代表完整 deploy phase management 已完成
- 目前 detumble 完成後不會自動切到下一個 mission phase
- 目前仍未加入 mission-level detumble-complete event；completion 仍由 ADCS subsystem event 與 mission telemetry 綜合判讀
- `Blocked-HW`：本次仍是 hosted replay autonomy evidence，不代表真實 ADCS hardware、真實 deployment disturbance、或在軌 dynamics 已驗證
