# Simulator 控制介面

EPS 與 ADCS simulator 提供 Unix socket sidecar，供測試與展示程式調整輸入。
這些控制只改變 simulator 狀態；OBC 的 mission command 仍經由 F Prime
command surface 執行。

## 啟動 Hosted Control Socket

```bash
EPS_SIM_CONTROL_SOCKET=/tmp/hosted-eps-sim-control.sock \
ADCS_SIM_CONTROL_SOCKET=/tmp/hosted-adcs-sim-control.sock \
  bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh
```

Target baseline 使用：

- EPS：`/tmp/subsystem-eps-sim-control.sock`
- ADCS：`/tmp/subsystem-adcs-sim-control.sock`

## EPS

設定電量：

```bash
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/hosted-eps-sim-control.sock \
  set-soc --value 80 --transition-sec 0
```

`--transition-sec 0` 代表立即設定；較大的值會產生漸進變化。

切換負載模型：

```bash
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/hosted-eps-sim-control.sock \
  set-load-mode --mode high-draw
```

可用模式為 `normal` 與 `high-draw`。

丟棄接下來四次 EPS status reply，以觸發 timeout/recovery：

```bash
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/hosted-eps-sim-control.sock \
  drop-status --count 4
```

將 `--count` 設為 `0` 可清除剩餘 counter。

## ADCS

丟棄接下來四次 ADCS state reply：

```bash
python3 scripts/chapter5_routes/adcs_sim_control.py \
  --socket /tmp/hosted-adcs-sim-control.sock \
  drop-state --count 4
```

重新開始 synthetic pointing pass：

```bash
python3 scripts/chapter5_routes/adcs_sim_control.py \
  --socket /tmp/hosted-adcs-sim-control.sock \
  restart-pointing-pass
```

`restart-pointing-pass` 只重設模擬軌跡；ADCS mode 由
`OBCApp.adcsBridge.ADCS_SET_MODE` 或 TTC entry policy 控制。

## Target 操作

在 subsystem host 上執行相同 control script：

```bash
ssh operator@subsystem.local '
cd /home/operator/lab/fprime/v0 &&
python3 scripts/chapter5_routes/eps_sim_control.py \
  --socket /tmp/subsystem-eps-sim-control.sock \
  set-soc --value 80 --transition-sec 0
'
```

ADCS 使用 `adcs_sim_control.py` 及
`/tmp/subsystem-adcs-sim-control.sock`。

## 檢查

Hosted socket：

```bash
ls /tmp/*eps*control*.sock /tmp/*adcs*control*.sock
```

若 socket 不存在，請用 `EPS_SIM_CONTROL_SOCKET` 或
`ADCS_SIM_CONTROL_SOCKET` 重新啟動 simulator。
