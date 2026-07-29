# Target 手動雙 GDS 操作手冊

Status: current target manual operator runbook.
Last reconciled during `manual-dual-gds-secure-ops-surface-v1` plus follow-up readiness repair on 2026-06-12.

這份手冊對應目前維護中的 target/lab 人工操作面，操作順序是：

1. 準備 shared remote target baseline
2. 啟動本機 macOS dual-GDS ground surface
3. 由操作者自行執行 secure auth、secure-v2 command、governed staged upload 與 governed `SEQ_*`

對目前 branch-head 的 Route 1 payload / sequence demo 來說，照這份手冊的
target 開機步驟做即可，不需要再額外手動套別的 auth preflight。前提是：

- `start_target_manual_baseline.sh` 與 `start_target_manual_ground_surface.sh`
  都是這個 branch 的 current 版本
- 沒有沿用舊的 `/tmp/manual-dual-gds/*` surface
- 開好後 manifest 內的 `targetAuthPreflight` 確實存在且 `applied=true`

## 適用範圍

本手冊涵蓋：

- target baseline readiness snapshot 與 ownership boundary
- target local dual-GDS 的 start/stop/status
- 使用 manual secure helper 對 target/lab path 做操作
- S-band auth、UHF re-auth、governed staged upload 與 governed `SEQ_*`

本手冊不涵蓋：

- 在一般 cleanup 流程中主動關閉 remote shared services
- GDS UI command interception
- RF 或非 lab deployment closure

## 1. 準備 Shared Target Baseline

實際開機順序建議直接照下面執行。

### 開機步驟 1: 準備 shared remote target baseline

```bash
bash scripts/manual_ops/target/start_target_manual_baseline.sh
```

這會包裝目前 governed shared-target readiness manager，並在
`/tmp/manual-dual-gds/target-baseline/` 寫出一份本機 snapshot manifest。
同時它也會套用 manual secure-auth 需要的 target preflight。
這一層只確認 target-local baseline readiness；它不會要求 fresh
`GROUND_LINK_UP`，因為這時候還沒有啟動本機 ground-side TCP client。
目前預設只開：

- `subsystem-sband-csp.service`
  - `COMM_NODE_INGRESS_DIAGNOSTICS=1`

`COMM_GROUNDLINK_DIAGNOSTICS` 預設不開。若你要額外看 OBC 端
`COMM ground link diagnostic: ...` journal，才顯式指定：

```bash
MANUAL_TARGET_OBC_GROUNDLINK_DIAGNOSTICS=1 \
bash scripts/manual_ops/target/start_target_manual_baseline.sh
```

governed target A-layer baseline 會先建立並驗證 scoped COMM CAN FD，
manual secure-auth preflight 只讀取其有效環境，不再擁有或移除這個
shared profile。對應值是：

- `COMM_CSP_SOCKETCAN_USE_CANFD=1`
- `COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST=5,6`
- `COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST=40`

若要顯式把相同 profile 交給 A，對應用法是：

```bash
TARGET_BASELINE_COMM_CSP_SOCKETCAN_USE_CANFD=1 \
TARGET_BASELINE_COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST=5,6 \
TARGET_BASELINE_COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST=40 \
bash scripts/manual_ops/target/start_target_manual_baseline.sh
```

A 會把 `57-csp-socketcan-canfd.conf` 只管理於：

- `obc-comm-csp-stack.service`
- `subsystem-sband-csp.service`
- `subsystem-uhf-csp.service`

不會去碰 `EPS/ADCS`。若你刻意覆寫成不帶這組條件，常見結果就是 secure
auth 卡在：

- `timed out waiting for CHALLENGE ...`

因為 challenge 雖然可能已在 OBC 端 issued，但 S-band data path 沒有真的把
對應封包穩定送到 ground oracle。

目前這條 baseline 一律要求 UHF readiness。也就是說，它會一起確認：
它會一起確認：

- `subsystem-uhf-csp.service`
- `subsystem-uhf-csp-stack.target`
- node `6` target baseline 已進入可切換狀態

功能情境可以只操作 S-band，但 shared baseline 不因此略過 UHF/node `6`
readiness。

若你需要額外放寬 timeout，再另外指定：

```bash
MANUAL_TARGET_COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS=1000
MANUAL_TARGET_COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS=1000
MANUAL_TARGET_COMM_GROUNDLINK_HEALTH_TIMEOUT_MS=1000
```

查看狀態：

```bash
bash scripts/manual_ops/target/status_target_manual_baseline.sh
```

如果你接下來是要做 payload Route 1 / sequence 類 demo，建議在這一步就順手確認
baseline manifest 內至少有：

- `targetAuthPreflight.applied=true`
- `observedObcEnvironment.COMM_CSP_SOCKETCAN_USE_CANFD=1`
- `observedSbandEnvironment.COMM_CSP_SOCKETCAN_USE_CANFD=1`
- `observedUhfEnvironment.COMM_CSP_SOCKETCAN_USE_CANFD=1`

如果你先前看到的是：

- `journal:obc-sband-availability-state:None`
- 或 ground surface startup timeout，且 `launcher.log` 內只有
  `CSP_OWNER_TIMEOUT` / `write() failed, we have been waiting for CAN buffers`

現在的 baseline manager 會先做一次 bounded repair：重啟
`obc-lab-can.service`，再重啟 `obc-comm-csp-stack.service`，用來清掉
OBC 端 `can0` 繼承到的 stale carrier 狀態。若這一步成功，新的 OBC
journal 會看到：

- `COMM_LINK_AVAILABILITY_CHANGED : Comm link SBAND (0) available 1`

之後再啟動本機 dual-GDS ground surface 即可。

目前的 stop 行為：

```bash
bash scripts/manual_ops/target/stop_target_manual_baseline.sh
```

這會退休本機 manual baseline metadata，並移除這次 manual secure-auth
baseline 套上的 preflight override；它不會停止 shared remote
OBC/subsystem/CAN services。

## 2. 啟動本機 Dual-GDS Ground Surface

### 開機步驟 2: 啟動本機 dual-GDS ground surface

```bash
GDS_UI_MODE=ui \
bash scripts/manual_ops/target/start_target_manual_ground_surface.sh
```

這個 entrypoint 現在會先跑一次：

```bash
bash scripts/ensure_ground_dual_gds_baseline.sh
```

也就是先做 `B-layer` 本機 ground residue 清場，再起新的 manual dual-GDS
surface。目的就是避免舊的 `ground_ttc_gateway` / `fprime-gds` 還佔著
node-`5` S-band TCP listener，導致新的 surface 雖然看起來起來了，
實際上 southbound 還是髒的。

目前預設也會自動找可用的本機 `gds` / `tts` / `gui` ports：

```bash
MANUAL_TARGET_GROUND_AUTO_PORTS=1
```

這是預設值。若你真的要固定 port，才另外指定：

```bash
MANUAL_TARGET_SBAND_GDS_PORT=51900
MANUAL_TARGET_SBAND_TTS_PORT=51901
MANUAL_TARGET_UHF_GDS_PORT=51910
MANUAL_TARGET_UHF_GDS_TTS_PORT=51911
MANUAL_TARGET_SBAND_GUI_PORT=5100
MANUAL_TARGET_UHF_GUI_PORT=5101
```

如果你想分開保留本機 operator 目錄，可用：

```bash
MANUAL_TARGET_GROUND_ROOT=/tmp/manual-dual-gds/target-ground-demo \
GDS_UI_MODE=ui \
bash scripts/manual_ops/target/start_target_manual_ground_surface.sh
```

查看狀態與 manifest：

```bash
bash scripts/manual_ops/target/status_target_manual_ground_surface.sh
cat /tmp/manual-dual-gds/target-ground/manifest.json
```

如果啟動時用了自訂 `MANUAL_TARGET_GROUND_ROOT`，後續 `status` 與 `stop`
也要帶同一個 root：

```bash
MANUAL_TARGET_GROUND_ROOT=/tmp/manual-dual-gds/target-ground-demo \
bash scripts/manual_ops/target/status_target_manual_ground_surface.sh
```

建議先確認：

- `surfaceRoot`
- `ownerPid`
- `lifecycleState`
- `gdsUiMode`
- `targetBaseline`
- `targetBaseline.targetAuthPreflight`
- `operatorSurfaces.sband.*`
- `operatorSurfaces.uhf.*`

如果你有帶 scoped COMM CAN FD，建議額外檢查
`targetBaseline.targetAuthPreflight.observedObcEnvironment`、
`observedSbandEnvironment`、`observedUhfEnvironment` 是否都真的看到：

- `COMM_CSP_SOCKETCAN_USE_CANFD=1`
- `COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST=5,6`
- `COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST=40`

`start_target_manual_ground_surface.sh` 現在的 ready 條件除了本機兩套
GDS/gateway listener 就緒外，也接受下列任一種 **fresh/current**
S-band readiness：

- target OBC 在這次 surface 啟動之後出現 fresh `groundLinkDriver` `UP`
- `sband-southbound-to-gds.bin` 已開始有 bytes
- `ground_ttc_gateway` 已穩定維持 `southbound-opened mode=tcp-client`

這樣做是為了避免兩種誤判：

- 只因沒有新的 `GROUND_LINK_UP` journal，就把其實已經穩定開好的 surface
  誤判成 startup failure
- 反過來把前一輪殘留的 stale `UP` 狀態，誤判成這一輪 ground surface
  已經真的接上 node `5`

### 開機步驟 3: 確認目前實際 port 與 southbound

```bash
cat /tmp/manual-dual-gds/target-ground/manifest.json
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
- `operatorSurfaces.<band>.southbound`
- `operatorSurfaces.<band>.fileStorageDir`

注意：

- `gdsPort` 是 GDS data port，不是瀏覽器要打開的 HTML GUI port
- 瀏覽器應該使用 `guiUrl`

## 3. 建立 Auth

### 開機步驟 4: 建立第一個 secure auth

通常先做 S-band：

S-band：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  auth establish
```

target helper 會先檢查：

- repo keystore SHA
- installed release bundled keystore SHA
- installed release manifest keystore SHA
- active OBC service `WorkingDirectory` 是否仍指向 governed current
  release root
- target baseline manifest 是否包含 manual secure-auth preflight
- remote `obc-comm-csp-stack.service` / `subsystem-sband-csp.service`
  是否仍保有對應的 preflight env

UHF 在顯式切換後使用：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  auth establish
```

如果你要在不切 primary band 的情況下，先驗證 bounded `uhf-backup`
adjunct auth，可直接用：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band uhf-backup \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  auth establish
```

## 4. 發送 Secure Command

### 開機步驟 5: 做第一個 command 測試

範例：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  command send OBCApp.modeManager.MODE_GET
```

查看 helper state：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  status
```

UHF backup adjunct 只支援 bounded read/status 類命令；不代表 UHF 在這個
階段已經取得 primary/high-authority。範例：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band uhf-backup \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  command send OBCApp.modeManager.MODE_GET
```

## 5. Governed Upload 與 `SEQ_*`

### 開機步驟 6: 開始後續功能測試

完成 shared baseline、local dual-GDS、S-band auth、第一個 command 之後，就可以依需求繼續：

- secure-v2 command
- `.sequence-staging/<leaf>` upload
- `SEQ_VALIDATE`
- `SEQ_RUN`
- `SEQ_PREPARE_MANUAL` / `SEQ_START` / `SEQ_STEP` / `SEQ_CANCEL`

若你要做 Route 1 類 payload demo，而且目前 spacecraft mode 還在 `SAFE`，先不要
直接送 `MODE_SET IDLE` / `MODE_SET PAYLOAD`。current target truth 需要先滿足
mode-safety 的 EPS gate：

1. 先把 EPS simulator SoC 拉到高值
2. 送一次 `OBCApp.epsBridge.EPS_GET_STATUS` 刷新 OBC cache
3. 再送 `OBCApp.modeManager.MODE_SET IDLE`
4. 再送 `OBCApp.modeManager.MODE_SET PAYLOAD`

這裡的成功 oracle 要看真實 `SYS_MODE_CHANGE`，不是只看 mode opcode completed。

若你要做目前維護中的 Route 1 payload / sequence 示範，可直接從這份 repo-owned
sequence source 開始：

- `scripts/manual_ops/examples/route1-demo.seq`

建議先在 UI 外 compile：

```bash
fprime-venv/bin/fprime-seqgen \
  --dictionary build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json \
  scripts/manual_ops/examples/route1-demo.seq \
  /tmp/route1-demo.bin
```

之後透過 governed upload 上傳剛剛編好的檔案：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  file upload /tmp/route1-demo.bin route1-demo.bin
```

- `destinationLeaf`: `route1-demo.bin`
- remote staged path truth: `.sequence-staging/route1-demo.bin`

接著再送：

- `SEQ_VALIDATE .sequence-staging/route1-demo.bin`
- `SEQ_RUN .sequence-staging/route1-demo.bin WAIT`

上傳：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
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
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  file upload ${REPO_ROOT}/scripts/manual_ops/examples/sample-sequence.bin sample-sequence.bin
```

驗證：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  seq validate sample-sequence.bin
```

手動 sequence 控制的意思是：

- `seq run ...` 會自動一路跑完整份 sequence
- `seq prepare-manual ...` 只先把 admitted copy 載入並建立 `context-id`
- 之後由你自己決定什麼時候 `seq start`
- 再用 `seq step` 一步一步推進每條 sequence command
- 中途可用 `seq cancel` 取消

手動 sequence 控制：

```bash
python scripts/manual_ops/manual_secure_ops.py --env target --band sband --manifest /tmp/manual-dual-gds/target-ground/manifest.json seq prepare-manual sample-sequence.bin
python scripts/manual_ops/manual_secure_ops.py --env target --band sband --manifest /tmp/manual-dual-gds/target-ground/manifest.json seq start <context-id>
python scripts/manual_ops/manual_secure_ops.py --env target --band sband --manifest /tmp/manual-dual-gds/target-ground/manifest.json seq step <context-id>
python scripts/manual_ops/manual_secure_ops.py --env target --band sband --manifest /tmp/manual-dual-gds/target-ground/manifest.json seq cancel <context-id>
```

## 6. UHF 切換與 Re-Auth

如果你顯式切換 active COMM band，helper 目前保存的 session 就不再有效。
之後若還要繼續發 secure-v2 command 或做 governed staged upload，必須先在
目標 runtime band 上重新 auth。

### Target S-band -> UHF primary

先用目前有效的 S-band session 送切換命令：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  command send OBCApp.commController.COMM_SET_ACTIVE UHF
```

這一步送完後：

- 原本的 `sband` session 失效
- 舊的 `uhf-backup` session 也不應再沿用
- 必須改用 `uhf-primary-after-failover` 重新 auth

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  auth establish
```

之後若要驗證 UHF primary secure command：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  command send OBCApp.modeManager.MODE_GET
```

### Target UHF primary -> S-band

若要切回 S-band，先用目前有效的 UHF primary session 送：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  command send OBCApp.commController.COMM_SET_ACTIVE SBAND
```

再重新建立 S-band auth：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  auth establish
```

## 7. 清理

建議關機順序：

### 關機步驟 1: 清掉 helper auth state

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band sband \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  auth clear
```

如果你有建立 UHF auth，也一併清掉：

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band uhf-backup \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  auth clear
```

```bash
python scripts/manual_ops/manual_secure_ops.py \
  --env target \
  --band uhf-primary-after-failover \
  --manifest /tmp/manual-dual-gds/target-ground/manifest.json \
  auth clear
```

### 關機步驟 2: 停掉本機 dual-GDS ground surface

```bash
bash scripts/manual_ops/target/stop_target_manual_ground_surface.sh
```

如果使用自訂 `MANUAL_TARGET_GROUND_ROOT`：

```bash
MANUAL_TARGET_GROUND_ROOT=/tmp/manual-dual-gds/target-ground-demo \
bash scripts/manual_ops/target/stop_target_manual_ground_surface.sh
```

這個 stop 會先要求 owner 結束，再依 manifest 內的實際 port / file-store
資訊補做 residue reap，清掉該 surface 產生的 `fprime-gds`、
`fprime_gds.executables.*`、`CustomDataHandlers`、`ground_ttc_gateway`
殘留子程序。

### 關機步驟 3: 如需退休本機 baseline metadata，再執行

```bash
bash scripts/manual_ops/target/stop_target_manual_baseline.sh
```

如果使用自訂 `MANUAL_TARGET_BASELINE_ROOT`：

```bash
MANUAL_TARGET_BASELINE_ROOT=/tmp/manual-dual-gds/target-baseline-demo \
bash scripts/manual_ops/target/stop_target_manual_baseline.sh
```

這個本機 cleanup 不會停止 shared remote baseline services。

## 非聲明範圍

- target manual baseline stop 不是 shared-service shutdown command
- stock GDS UI 不是 secure-command send plane
- 這份手冊不把 target/lab path 擴大宣稱成 RF 或 final-flight deployment closure
