# 測試紀錄：scenario-driven-sim-architecture-v1

- 日期：2026-04-03
- 層級：L2 / L3
- 環境：
  - host: macOS development machine
  - mode: hosted simulator replay
- 關聯變更：`openspec/changes/scenario-driven-sim-architecture-v1/`
- 關聯文件：
  - `openspec/changes/scenario-driven-sim-architecture-v1/specs/scenario-driven-validation/spec.md`
  - `openspec/changes/scenario-driven-sim-architecture-v1/specs/eps-subsystem/spec.md`
  - `openspec/changes/scenario-driven-sim-architecture-v1/specs/adcs-subsystem/spec.md`
  - `openspec/changes/scenario-driven-sim-architecture-v1/specs/verification-evidence/spec.md`

## 1. 目標

本 change 的目標是建立第一版 repository-owned scenario bridge 架構，讓後續 autonomy / mission-driven extension 能以 offline replay 驅動 hosted simulators，而不是把 STK 或 scenario truth 直接餵進 OBC runtime。

本次重點是：

- repository-owned offline replay CSV contract
- `ScenarioTimeline` / `ScenarioBridge`
- EPS scenario-driven sunlight + battery SoC replay
- ADCS deployment-rate seeding，但不覆寫後續 detumble dynamics

本次仍然**不**處理：

- direct STK-to-OBC integration
- MissionExecutive / autonomy policy
- housekeeping logging/downlink
- ground-pass command routing
- RF / vendor radio / `csp-es` integration

## 2. 本次實作重點

- 新增 `simulators/scenario/ScenarioTimeline.*`
  - 載入 repository-owned replay CSV
  - 用 zero-order hold 依 replay time 取樣
- 新增 `simulators/scenario/ScenarioBridge.*`
  - 持有當前 replay sample
  - 持續把 sunlight / battery SoC 套用到 EPS simulator
  - 僅在 initialization 時把 angular rates seed 到 ADCS simulator
- `EpsSimModel` 新增 scenario state hook
- `AdcsSimModel` 新增 scenario angular-rate seed hook，並把 derived-state refresh 與 step dynamics 分離
- 新增 `scenario_bridge_integration_test`
  - 驗證 offline replay 會更新 EPS state
  - 驗證 ADCS 可由 scenario seed deployment-rate，之後仍能被 `DETUMBLE` 收斂
  - 驗證後續 replay step 不會把 ADCS rates 強制覆回去

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

### 3.3 直接執行新的 scenario replay test

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/build-fprime-automatic-native-ut/bin/Darwin/scenario_bridge_integration_test
```

### 3.4 跑完整本機 regression

```bash
PATH=$REPO_ROOT/fprime-venv/bin:$PATH \
$REPO_ROOT/fprime-venv/bin/fprime-util check --all
```

## 4. 觀察結果

### 4.1 Scenario contract

- repository-owned example contract 已落在 `simulators/scenario/examples/offline_replay_v1.csv`
- fields:
  - `time_sec`
  - `sunlight`
  - `battery_soc_pct`
  - `ground_pass_open`
  - `link_available`
  - `omega_x_rad_s`
  - `omega_y_rad_s`
  - `omega_z_rad_s`

### 4.2 Scenario bridge behavior

- `ScenarioTimeline` 成功載入 replay CSV，並以 zero-order hold 提供 sample lookup
- `ScenarioBridge` initialization 會：
  - 套用第一筆 EPS sunlight / SoC
  - seed 第一筆 ADCS deployment-rate
- 後續 `stepTo()` 會：
  - 更新 EPS sunlight / SoC
  - 保留 `ground_pass_open` / `link_available` 作為 bridge current state
  - **不**重新覆寫 ADCS angular rates

### 4.3 測試結果

- `scenario_bridge_integration_test`: `PASS`
- `fprime-util check --all`: `PASS`
  - `eps_zmq_integration_test`
  - `adcs_zmq_integration_test`
  - `comm_transport_integration_test`
  - `scenario_bridge_integration_test`
  - 9 個 OBC component UT

## 5. 驗收結論

- `PASS`：repository-owned offline replay contract 已建立
- `PASS`：scenario bridge 已可驅動 EPS / ADCS simulator 的第一版 scenario inputs
- `PASS`：ADCS deployment-rate seeding 不會破壞後續 detumble control-response behavior
- `PASS`：本 change 未破壞既有 simulator integration tests 與 OBC component UT baseline

## 6. 邊界與限制

- 目前 `ground_pass_open` / `link_available` 只保留在 bridge state，尚未接到 comm path 或 autonomy layer
- 目前 battery SoC replay 是 early SIL validation 用的 exogenous input，不是高擬真的 battery energy model
- 目前 ADCS scenario replay 僅 seed 初始 angular rate，不做連續姿態 truth 強制覆寫
- `Blocked-HW`：本次未涉及真實 hardware-in-the-loop、Raspberry Pi target replay、真實 radio 或 STK exporter
