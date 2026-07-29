# Hosted 手動雙 GDS 操作手冊

Status: current hosted manual operator runbook.
Last reconciled during `manual-dual-gds-secure-ops-surface-v1` on 2026-06-07.

這份手冊對應目前維護中的 hosted 人工操作面，目的是：

- 打開兩套 stock GDS
- 維持 shared hosted `TopCcsds` runtime 運作
- 之後由操作者自行執行 secure auth、secure-v2 command、governed staged upload 與 governed `SEQ_*`

## 適用範圍

本手冊涵蓋：

- 啟動與停止 hosted manual dual-GDS surface
- 讀取 surface 的 manifest/status contract
- 在 S-band 或 UHF 上建立 secure auth
- 透過 repo-owned helper 發送 secure-v2 command
- 只上傳 governed `.sequence-staging/<leaf>` 檔案
- 執行 governed `SequenceAdmissionController` wrapper commands

本手冊不涵蓋：

- 直接在 stock GDS UI 點 command 並期待自動 secure wrapping
- one-GDS aggregation 或 one-gateway multiplexing
- target/lab remote service 管理
- payload preview/raw 或更廣的 mission-ops closure

## 啟動

實際開機順序建議直接照下面執行。

### 開機步驟 1: 啟動 hosted manual surface

最常用指令：

```bash
GDS_UI_MODE=ui \
bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh
```

如果你想保留獨立的工作目錄與 runtime 目錄，使用：

```bash
MANUAL_HOSTED_SURFACE_ROOT=/tmp/manual-dual-gds/hosted-demo \
MANUAL_HOSTED_RUNTIME_ROOT=/tmp/manual-dual-gds/runtime-demo \
MANUAL_HOSTED_AUTO_PORTS=1 \
GDS_UI_MODE=ui \
bash scripts/manual_ops/hosted/start_hosted_manual_surface.sh
```

常用參數說明：

- `MANUAL_HOSTED_SURFACE_ROOT=/tmp/manual-dual-gds/hosted`
- `MANUAL_HOSTED_RUNTIME_ROOT=/tmp/manual-dual-gds/runtime`
- `MANUAL_HOSTED_AUTO_PORTS=1`
- `GDS_UI_MODE=ui|headless`
- `MANUAL_HOSTED_COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS=1000`

目前 hosted manual surface 預設會把 OBC 端
`COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS` 放寬到 `1000 ms`，避免 shared
hosted dual-GDS path 在啟動後因 `UPLINK_POLL` timeout 過緊而出現早期
`CSP_OWNER_TIMEOUT` / `GROUND_LINK_DOWN` 片段。若要做更激進或更保守的診斷，
再顯式覆寫這個值。

### 開機步驟 2: 確認 surface 已經 ready

```bash
bash scripts/manual_ops/hosted/status_hosted_manual_surface.sh
```

目前 hosted manual surface 會等 shared hosted OBC runtime 觀察到 fresh
`GROUND_LINK_UP` 才宣告 ready；也就是說，這一層不只本機 listener 起來，
還包含 stock GDS / `ground_ttc_gateway` / hosted OBC 之間的 ground path
已經接通。

如果要直接看 manifest：

```bash
cat /tmp/manual-dual-gds/hosted/manifest.json
```

如果你啟動時用了自訂 `MANUAL_HOSTED_SURFACE_ROOT`，後續 `status` 與 `stop`
也要帶同一個 root：

```bash
MANUAL_HOSTED_SURFACE_ROOT=/tmp/manual-dual-gds/hosted-demo \
bash scripts/manual_ops/hosted/status_hosted_manual_surface.sh
```

### 開機步驟 3: 找出目前實際 port

建議先從 manifest 看目前 auto-assigned port，再做後續操作：

```bash
cat /tmp/manual-dual-gds/hosted/manifest.json
```

重點欄位：

- `operatorSurfaces.sband.gdsPort`
- `operatorSurfaces.sband.gdsTtsPort`
- `operatorSurfaces.sband.guiPort`
- `operatorSurfaces.sband.guiUrl`
- `operatorSurfaces.uhf.gdsPort`
- `operatorSurfaces.uhf.gdsTtsPort`
- `operatorSurfaces.uhf.guiPort`
- `operatorSurfaces.uhf.guiUrl`
- `operatorSurfaces.<band>.fileStorageDir`

注意：

- `gdsPort` 是 GDS data port，不是瀏覽器要打開的 HTML GUI port
- 瀏覽器應該使用 `guiUrl`

### 開機步驟 4: 建立第一個 secure auth

通常先做 S-band：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth establish
```

### 開機步驟 5: 做第一個 command 測試

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  command send OBCApp.modeManager.MODE_GET
```

### 開機步驟 6: 開始後續功能測試

完成上面兩步後，就可以依需求繼續：

- secure-v2 command
- `.sequence-staging/<leaf>` upload
- `SEQ_VALIDATE`
- `SEQ_RUN`
- `SEQ_PREPARE_MANUAL` / `SEQ_START` / `SEQ_STEP` / `SEQ_CANCEL`

## 查看狀態與 Manifest

```bash
bash scripts/manual_ops/hosted/status_hosted_manual_surface.sh
cat /tmp/manual-dual-gds/hosted/manifest.json
```

建議先確認以下欄位：

- `surfaceRoot`
- `ownerPid`
- `lifecycleState`
- `gdsUiMode`
- `sharedHostedRuntime`
- `operatorSurfaces.sband.gdsPort`
- `operatorSurfaces.sband.gdsTtsPort`
- `operatorSurfaces.sband.guiUrl`
- `operatorSurfaces.uhf.gdsPort`
- `operatorSurfaces.uhf.gdsTtsPort`
- `operatorSurfaces.uhf.guiUrl`
- `operatorSurfaces.<band>.fileStorageDir`
- `operatorSurfaces.<band>.southbound`
- `nonClaims`

## 建立 Auth

S-band：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth establish
```

UHF 在顯式切換後使用：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth establish
```

如果你要在不切 primary band 的情況下，先驗證 bounded `uhf-backup`
adjunct auth，可直接用：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band uhf-backup \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth establish
```

目前會使 session 失效的條件：

- `auth clear`
- surface restart
- inactivity timeout
- 顯式 band switch，例如 `COMM_SET_ACTIVE`

## 發送 Secure Command

S-band 範例：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  command send OBCApp.modeManager.MODE_GET
```

查看 helper 保存的 state：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  status
```

UHF backup adjunct 只支援 bounded read/status 類命令；不代表 UHF 在這個
階段已經取得 primary/high-authority。範例：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band uhf-backup \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  command send OBCApp.modeManager.MODE_GET
```

## Primary Band 切換

### Hosted S-band -> UHF primary

先用目前有效的 S-band session 送切換命令：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  command send OBCApp.commController.COMM_SET_ACTIVE UHF
```

這一步送完後：

- 原本的 `sband` session 失效
- 舊的 `uhf-backup` session 也不應再沿用
- 必須改用 `uhf-primary-after-failover` 重新 auth

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth establish
```

之後若要驗證 UHF primary secure command：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  command send OBCApp.modeManager.MODE_GET
```

### Hosted UHF primary -> S-band

若要切回 S-band，先用目前有效的 UHF primary session 送：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  command send OBCApp.commController.COMM_SET_ACTIVE SBAND
```

再重新建立 S-band auth：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth establish
```

## 上傳檔案與 Governed Sequence 操作

只允許上傳到 governed staging family：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  file upload ${REPO_ROOT}/scripts/manual_ops/examples/sample.bin sample.bin
```

`scripts/manual_ops/examples/sample.bin` 只用來做 `file upload` smoke；它不
是有效的 sequencer binary。

若你要測 sequence wrapper，請改用：

- `${REPO_ROOT}/scripts/manual_ops/examples/sample-sequence.seq`
- `${REPO_ROOT}/scripts/manual_ops/examples/sample-sequence.bin`

其中：

- `.seq` 是人可讀的原始 sequence source
- `.bin` 是已經用 `fprime-seqgen` 編好的有效 sequencer binary

先把 sequence binary 上傳到 governed staged path：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  file upload ${REPO_ROOT}/scripts/manual_ops/examples/sample-sequence.bin sample-sequence.bin
```

驗證：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  seq validate sample-sequence.bin
```

直接執行：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  seq run sample-sequence.bin --run-mode NO_WAIT
```

手動 sequence 控制的意思是：

- `seq run ...` 會自動一路跑完整份 sequence
- `seq prepare-manual ...` 只先把 admitted copy 載入並建立 `context-id`
- 之後由你自己決定什麼時候 `seq start`
- 再用 `seq step` 一步一步推進每條 sequence command
- 中途可用 `seq cancel` 取消

手動步進流程：

```bash
python scripts/manual_ops/manual_secure_ops.py --env hosted --band sband --manifest /tmp/manual-dual-gds/hosted/manifest.json seq prepare-manual sample-sequence.bin
python scripts/manual_ops/manual_secure_ops.py --env hosted --band sband --manifest /tmp/manual-dual-gds/hosted/manifest.json seq start <context-id>
python scripts/manual_ops/manual_secure_ops.py --env hosted --band sband --manifest /tmp/manual-dual-gds/hosted/manifest.json seq step <context-id>
python scripts/manual_ops/manual_secure_ops.py --env hosted --band sband --manifest /tmp/manual-dual-gds/hosted/manifest.json seq cancel <context-id>
```

## 清理

建議關機順序：

### 關機步驟 1: 清掉 helper auth state

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band sband \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth clear
```

如果你有建立 UHF auth，也一併清掉：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band uhf-backup \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth clear
```

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env hosted \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/hosted/manifest.json \
  auth clear
```

### 關機步驟 2: 停掉 hosted manual surface

```bash
bash scripts/manual_ops/hosted/stop_hosted_manual_surface.sh
```

如果使用自訂 `MANUAL_HOSTED_SURFACE_ROOT`：

```bash
MANUAL_HOSTED_SURFACE_ROOT=/tmp/manual-dual-gds/hosted-demo \
bash scripts/manual_ops/hosted/stop_hosted_manual_surface.sh
```

這個 stop 會先要求 owner 結束，再依 manifest 內的實際 port / runtime-root
資訊補做 residue reap，清掉 hosted manual surface 殘留的
`fprime-gds`、`ground_ttc_gateway`、`csp_zmqproxy`、hosted `OBC`
以及 orphaned simulator helpers。

## 非聲明範圍

- stock GDS UI 不是 secure-command send plane
- 這個 surface 不聲明 one-GDS aggregation 或 one-gateway multiplexing
- 這份手冊不取代 hosted proof wrappers 作為 closeout evidence
