# 測試紀錄：rpi-target-integration-v1

- 日期：2026-03-30
- 層級：L3 / L4
- 環境：
  - host: macOS development machine
  - target: Raspberry Pi 3 Model B+ via `ssh operator@youjun.local`
- 關聯變更：`openspec/changes/rpi-target-integration-v1/`
- 關聯文件：
  - `openspec/changes/rpi-target-integration-v1/specs/platform-baseline/spec.md`
  - `openspec/changes/rpi-target-integration-v1/specs/comm-subsystem/spec.md`
  - `openspec/changes/rpi-target-integration-v1/specs/boot-update/spec.md`
  - `openspec/changes/rpi-target-integration-v1/specs/verification-evidence/spec.md`

> Historical note (2026-05-27): this record predates the current host-role naming.
> Any `operator@youjun.local` reference below is historical evidence only.
> Current governed OBC target references are `operator@obc.local` or `operator@<private-lab-host>`.

## 1. 目標

驗證先前標記為 `Deferred-RPi` 的目標整合項目已實際在 Raspberry Pi 3B+ 上執行，包含：

- governed workspace sync / native build
- `integ-rpi` stack launch
- target-side GDS TCP 回連
- boot metadata 跨 process restart 保留 pending confirm / rollback 狀態

## 2. 執行指令

### 2.1 同步與建置

```bash
bash scripts/sync_rpi_workspace.sh
bash scripts/bootstrap_rpi_workspace.sh
```

觀察重點：

- workspace 同步至 `$OBC_HOME/lab/fprime/v0`
- Pi 上完成 native generate / build
- 目標可執行檔位於 `$OBC_HOME/lab/fprime/v0/build-fprime-automatic-native/bin/Linux/OBC`

### 2.2 Boot restart probe

```bash
bash scripts/run_rpi_boot_probe.sh
```

關鍵觀察：

- staged image size: `25`
- staged image digest:
  `9b983e5c144c041a2512e7e604cd8858cfa045108b533a38c9e3ef7798b677de`
- `BOOT_PREPARE_UPDATE`、`BOOT_VERIFY_STAGED_IMAGE`、`BOOT_ACTIVATE_STAGED_IMAGE` 均回傳 `response=0`

`activate` 後 metadata：

```text
active_slot=SLOT_B
pending_slot=SLOT_B
last_known_good_slot=SLOT_A
confirmed=0
expected_digest=9b983e5c144c041a2512e7e604cd8858cfa045108b533a38c9e3ef7798b677de
expected_size=25
last_boot_attempt_time=3
last_error_code=0
stage_verified=1
staged_path=$OBC_HOME/lab/fprime/v0/runtime/boot-restart-probe/staging/staged-image.bin
```

restart 後 `boot status` 摘要：

```text
boot active=SLOT_B pending=SLOT_B confirmed=no progress=100 confirmTimeoutRemaining=59
```

rollback 後摘要：

```text
boot active=SLOT_A pending=NONE confirmed=yes progress=0 confirmTimeoutRemaining=0
```

結論：

- Pi target 上的 `BootManager` 會自 configured persistent root 重新載入 metadata
- pending slot 與 confirm window 會跨 process restart 保留
- operator 仍可在 restart 後完成 rollback

### 2.3 Pi target 連線回本機 GDS

本機：

```bash
PATH="$PWD/fprime-venv/bin:$PATH" fprime-gds -n -g none --framing-selection fprime --dictionary "$PWD/build-artifacts/Darwin/OBC/dict/AppTopologyDictionary.json" --ip-address 0.0.0.0 --ip-port 50001 --log-to-stdout
```

Pi target stack：

```bash
GDS_PORT=50001 bash scripts/run_rpi_stack.sh
```

為了在證據中捕捉狀態，也執行了等價的非互動 command feed，確認 `status` 輸出：

```text
ground enabled=yes target=<private-lab-host>:50001
radioLink connected=yes tx=14 rx=120 txErr=0 rxErr=0
```

本機 GDS 同步觀察到：

```text
Server connected to 0.0.0.0:50001
```

結論：

- `integ-rpi` stack 可由 repo-local helper 啟動
- Pi 上的 OBC 會以 TCP client 身分回連本機 `fprime-gds`
- comm / ground path 沿用 shared controller logic，未分叉專用 target business logic

## 3. 觀察與限制

- `scripts/sync_rpi_workspace.sh` 會因 macOS tar xattr 顯示 `LIBARCHIVE.xattr.com.apple.provenance` 類 warning；不影響同步結果
- 同步到 Pi 的 governed workspace 不包含 `.git` metadata，因此 F' version component 在 target build 中會退回 framework 的 fallback version 字串；這不影響本次功能驗證，但 version 顯示值不應被誤解為 framework baseline 已變更
- 在最初的 target rollback 驗證中，`metadata-v1.txt` 曾觀察到 `staged_path` 清空後留下舊尾巴；此問題已由 follow-up change `boot-metadata-clean-write-fix` 修正，重新驗證後 rollback metadata 末尾為乾淨的 `staged_path=`
- 實體 UART 線路與真實 radio hardware path 仍屬 `Blocked-HW`
- 本次驗證的是 target-side process restart 與 file-backed boot state，不宣稱已完成真正的 Raspberry Pi bootloader / partition handoff / power-loss recovery 驗證

## 4. 判定

- Raspberry Pi native build：Pass
- Raspberry Pi integrated stack launch：Pass
- Raspberry Pi target -> host `fprime-gds` connectivity：Pass
- Boot metadata restart persistence on target hardware：Pass
- Remaining real UART / radio hardware validation：Blocked-HW
