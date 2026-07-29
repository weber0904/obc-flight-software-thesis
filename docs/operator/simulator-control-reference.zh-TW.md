# Simulator Control Reference

狀態：current manual sidecar control 參考
更新日期：2026-07-10

這份文件整理 **repo 目前已存在的 simulator 外部控制介面**。
它們都屬於 simulator-owned sidecar surface，不是 flight software public command。

為了避免 hosted 與 target 指令混在一起，這份文件固定分成兩類：

- `target`
  - simulator 跑在 `subsystem.local`
  - control socket 也在 `subsystem.local`
  - 通常透過 `ssh operator@subsystem.local '...'` 操作
- `hosted`
  - simulator 跑在本機 `macOS`
  - control socket 也在本機
  - 直接在 repo root 下送 `python3 scripts/...` 操作

## 共用前提

以下範例都假設 repo root 是：

- `${REPO_ROOT}`

## Socket 路徑與啟動方式

### Target

目前 target baseline 預設 control socket 路徑是：

- subsystem workspace 在 `subsystem.local:/home/operator/lab/fprime/v0`
- EPS: `/tmp/subsystem-eps-sim-control.sock`
- ADCS: `/tmp/subsystem-adcs-sim-control.sock`

也就是說，只要 target baseline 正常起來，通常不需要另外指定 socket path。

### Hosted

hosted 沒有像 target 一樣的固定 baseline 預設 socket。

hosted `eps_simulator` / `adcs_simulator` 只有在啟動前明確帶：

- `EPS_SIM_CONTROL_SOCKET`
- `ADCS_SIM_CONTROL_SOCKET`

才會真的建立 control socket。

建議 hosted 手動操作時固定自己指定，例如：

```bash
EPS_SIM_CONTROL_SOCKET=/tmp/hosted-eps-sim-control.sock \
ADCS_SIM_CONTROL_SOCKET=/tmp/hosted-adcs-sim-control.sock \
bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh
```

如果不是跑 manual hosted surface，而是直接跑 hosted stack，也可以：

```bash
EPS_SIM_CONTROL_SOCKET=/tmp/hosted-eps-sim-control.sock \
ADCS_SIM_CONTROL_SOCKET=/tmp/hosted-adcs-sim-control.sock \
bash scripts/run_dev_stack.sh
```

之後就用這兩條本機 socket 操作：

- EPS: `/tmp/hosted-eps-sim-control.sock`
- ADCS: `/tmp/hosted-adcs-sim-control.sock`

## EPS simulator

### 1. `set-soc`

用途：

- mode-safety 門檻刺激
- payload precondition
- TTC precondition

### Target 範例

```bash
ssh operator@subsystem.local '
cd /home/operator/lab/fprime/v0 &&
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/subsystem-eps-sim-control.sock \
  set-soc --value 80 --transition-sec 0
'
```

### Hosted 範例

```bash
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/hosted-eps-sim-control.sock \
  set-soc --value 80 --transition-sec 0
```

參數：

- `--value`
  - 目標 SoC 百分比
- `--transition-sec`
  - `0` 代表立即跳值
  - 非 `0` 代表 lazy ramp

### 2. `set-load-mode`

用途：

- demo load switching
- 只改 simulator 內部生成的負載曲線
- **不**等於 fault injection

可用 mode：

- `normal`
- `high-draw`

### Target 範例

```bash
ssh operator@subsystem.local '
cd /home/operator/lab/fprime/v0 &&
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/subsystem-eps-sim-control.sock \
  set-load-mode --mode high-draw
'
```

### Hosted 範例

```bash
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/hosted-eps-sim-control.sock \
  set-load-mode --mode high-draw
```

### 3. `drop-status`

用途：

- Route 3 EPS `R3/R5` proof trigger
- 讓接下來的 `EPS STATUS` replies 被 simulator 丟掉
- 這才是 current maintained EPS timeout 觸發法

### Target 範例

```bash
ssh operator@subsystem.local '
cd /home/operator/lab/fprime/v0 &&
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/subsystem-eps-sim-control.sock \
  drop-status --count 4
'
```

### Hosted 範例

```bash
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/hosted-eps-sim-control.sock \
  drop-status --count 4
```

清除剩餘 drop counter：

#### Target

```bash
ssh operator@subsystem.local '
cd /home/operator/lab/fprime/v0 &&
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/subsystem-eps-sim-control.sock \
  drop-status --count 0
'
```

#### Hosted

```bash
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/hosted-eps-sim-control.sock \
  drop-status --count 0
```

## ADCS simulator

### 1. `drop-state`

用途：

- Route 3 ADCS `R3` proof trigger
- 讓接下來的 `ADCS STATE` replies 被 simulator 丟掉

### Target 範例

```bash
ssh operator@subsystem.local '
cd /home/operator/lab/fprime/v0 &&
python3 scripts/chapter5_routes/adcs_sim_control.py \
  --socket /tmp/subsystem-adcs-sim-control.sock \
  drop-state --count 4
'
```

### Hosted 範例

```bash
python3 scripts/chapter5_routes/adcs_sim_control.py \
  --socket /tmp/hosted-adcs-sim-control.sock \
  drop-state --count 4
```

清除剩餘 drop counter：

#### Target

```bash
ssh operator@subsystem.local '
cd /home/operator/lab/fprime/v0 &&
python3 scripts/chapter5_routes/adcs_sim_control.py \
  --socket /tmp/subsystem-adcs-sim-control.sock \
  drop-state --count 0
'
```

#### Hosted

```bash
python3 scripts/chapter5_routes/adcs_sim_control.py \
  --socket /tmp/hosted-adcs-sim-control.sock \
  drop-state --count 0
```

### 2. `restart-pointing-pass`

用途：

- 只把 synthetic `POINTING` pass profile rewind
- **不**直接切換 ADCS mode

### Target 範例

```bash
ssh operator@subsystem.local '
cd /home/operator/lab/fprime/v0 &&
python3 scripts/chapter5_routes/adcs_sim_control.py \
  --socket /tmp/subsystem-adcs-sim-control.sock \
  restart-pointing-pass
'
```

### Hosted 範例

```bash
python3 scripts/chapter5_routes/adcs_sim_control.py \
  --socket /tmp/hosted-adcs-sim-control.sock \
  restart-pointing-pass
```

## Hosted 現場快速檢查

如果你已經起好 hosted surface / hosted stack，但忘了自己用哪條 socket，
先查本機是否真的有建立：

```bash
ls /tmp/*eps*control*.sock /tmp/*adcs*control*.sock 2>/dev/null
```

若這裡完全沒東西，通常代表：

- 啟動 hosted stack 前沒有指定 `EPS_SIM_CONTROL_SOCKET`
- 或啟動 hosted stack 前沒有指定 `ADCS_SIM_CONTROL_SOCKET`

這時候不是 control script 壞掉，而是 simulator 根本沒有開 sidecar socket。

## 目前沒有的外部控制

- 沒有「外部直接切 ADCS `POINTING` mode」的 simulator socket command
- current truth 是：
  - 若要手動切 ADCS mode，用 OBC public command `OBCApp.adcsBridge.ADCS_SET_MODE ...`
  - 若要走 Route 2 current TTC path，`POINTING` 應由 TTC entry 後的 internal hook 自動觸發

## Source distinction

這些 control socket command 只證明：

- external operator 可以改 simulator 行為

它們不證明：

- ground 已經透過 Mission Console 看到相同資訊
- OBC public command surface 本身具備這些 sidecar-only 控制
