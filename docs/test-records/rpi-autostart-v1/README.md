# 測試紀錄：rpi-autostart-v1

- 日期：2026-03-30
- 層級：L3 / L4
- 環境：
  - host: macOS development machine
  - target: Raspberry Pi 3 Model B+ via `ssh operator@youjun.local`
- 關聯變更：`openspec/changes/rpi-autostart-v1/`
- 關聯文件：
  - `openspec/changes/rpi-autostart-v1/specs/platform-baseline/spec.md`
  - `openspec/changes/rpi-autostart-v1/specs/boot-update/spec.md`
  - `openspec/changes/rpi-autostart-v1/specs/verification-evidence/spec.md`

> Historical note (2026-05-27): this record predates the current host-role naming.
> Any `operator@youjun.local` reference below is historical evidence only.
> Current governed OBC target references are `operator@obc.local` or `operator@<private-lab-host>`.

## 1. 目標

驗證 governed Raspberry Pi install root 已不只可手動啟動，也能：

- 由 repo-local helper 安裝 systemd service
- 以 installed `current` release 作為正式 target 啟動入口
- 在沒有 interactive stdin 的情況下，以 headless 模式持續執行 OBC
- 在 OS reboot 後自動重新拉起 installed stack
- 在 reboot 後仍可檢視 release pointer、version metadata、boot metadata 與 systemd/journal 證據

## 2. 執行指令

### 2.1 安裝 bundle 與 service

先安裝本次變更對應的 installed release：

```bash
bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/5a8f17d-dirty/obc-rpi-5a8f17d-dirty.tar.gz
```

結果：

```text
Installed release 5a8f17d-dirty at $OBC_HOME/obc-deploy
```

再安裝並啟用 governed autostart service：

```bash
bash scripts/install_rpi_autostart.sh
```

關鍵輸出：

```text
Created symlink /etc/systemd/system/multi-user.target.wants/obc-installed-stack.service -> /etc/systemd/system/obc-installed-stack.service
Loaded: loaded (/etc/systemd/system/obc-installed-stack.service; enabled; preset: enabled)
Active: active (running)
CGroup: /system.slice/obc-installed-stack.service
  bash $OBC_HOME/obc-deploy/current/launch/run_stack.sh
  $OBC_HOME/obc-deploy/current/bin/eps_simulator ...
  $OBC_HOME/obc-deploy/current/bin/adcs_simulator ...
  $OBC_HOME/obc-deploy/current/bin/radio_mock_server ...
```

### 2.2 安裝後狀態檢查

```bash
bash scripts/rpi_autostart_status.sh
```

關鍵觀察：

```text
is-enabled: enabled
is-active: active
OBC --comm tcp --comm-host 127.0.0.1 --comm-port 7000 ... --headless
Framework Version: [v4.1.0]
Project Version: [5a8f17d-dirty]
Runtime mode: headless
```

對 install root 與 runtime tree 另做核對：

```bash
ssh operator@youjun.local "readlink -f $OBC_HOME/obc-deploy/current && cat $OBC_HOME/obc-deploy/current/meta/version.json && find $OBC_HOME/obc-deploy/runtime/integ-rpi -maxdepth 2 -type d | sort"
```

結果摘要：

```text
current -> $OBC_HOME/obc-deploy/releases/5a8f17d-dirty
framework_version=v4.1.0
project_version=5a8f17d-dirty
$OBC_HOME/obc-deploy/runtime/integ-rpi/logs
$OBC_HOME/obc-deploy/runtime/integ-rpi/persistent-data
$OBC_HOME/obc-deploy/runtime/integ-rpi/staging
```

### 2.3 OS reboot 後自動拉起驗證

第一次以預設 `youjun.local` 執行 reboot probe 時，target 實際已 reboot 並重新啟動 service，但 host 端 `.local` mDNS 解析恢復較慢，導致 probe helper 在等待 SSH 回線時 timeout。直接檢查 target 可看到：

```text
last reboot -> Mon Mar 30 05:59 / 06:00 / 10:19
obc-installed-stack.service -> active
```

因此正式可重現的 reboot evidence 改用 helper 已支援的 `RPI_SSH_TARGET` 覆寫：

```bash
RPI_SSH_TARGET=operator@<private-lab-host> bash scripts/run_rpi_autostart_probe.sh
```

關鍵輸出：

```text
active
Loaded: loaded (/etc/systemd/system/obc-installed-stack.service; enabled; preset: enabled)
Active: active (running) since Mon 2026-03-30 10:19:17 CST
Started obc-installed-stack.service - OBC Installed Stack.
=== installed processes ===
$OBC_HOME/obc-deploy/current/bin/eps_simulator ...
$OBC_HOME/obc-deploy/current/bin/adcs_simulator ...
$OBC_HOME/obc-deploy/current/bin/radio_mock_server ...
```

probe 完成後再補抓 steady-state 狀態：

```bash
ssh operator@<private-lab-host> "sudo systemctl --no-pager --full status obc-installed-stack.service | sed -n '1,25p'; ps -ef | grep -E 'obc-deploy/(current|releases/.+)/bin/(eps_simulator|adcs_simulator|radio_mock_server|OBC)' | grep -v grep || true"
```

結果摘要：

```text
Active: active (running)
bash $OBC_HOME/obc-deploy/current/launch/run_stack.sh
$OBC_HOME/obc-deploy/current/bin/eps_simulator ...
$OBC_HOME/obc-deploy/current/bin/adcs_simulator ...
$OBC_HOME/obc-deploy/current/bin/radio_mock_server ...
$OBC_HOME/obc-deploy/current/bin/OBC ... --headless
```

### 2.4 Boot metadata 與 release pointer 檢查

```bash
ssh operator@<private-lab-host> "cat $OBC_HOME/obc-deploy/runtime/integ-rpi/persistent-data/boot/metadata-v1.txt"
```

結果：

```text
active_slot=SLOT_A
pending_slot=NONE
last_known_good_slot=SLOT_A
confirmed=1
expected_digest=
expected_size=0
last_boot_attempt_time=0
last_error_code=0
stage_verified=0
staged_path=
```

## 3. 觀察與調整

- OBC runtime 新增 `--headless` 模式後，可在 closed stdin / `StandardInput=null` 的 systemd 環境下維持執行，不會因 REPL EOF 而退出。
- service 固定從 `$OBC_HOME/obc-deploy/current/launch/run_stack.sh` 啟動，而非 synced source workspace，符合 install-root 受管部署目標。
- `.local` mDNS 解析在 reboot 後可能比 target service 恢復得慢；這不影響 target autostart 本身，但會影響 host 端 probe helper 的回線等待。因此 helper 與文件都保留 `RPI_SSH_TARGET` 覆寫能力，必要時可直接指定 LAN IP。
- 本 change 驗證的是 OS reboot 後的 service-managed startup，不宣稱已完成 Raspberry Pi firmware、bootloader、partition handoff 或 power-loss recovery。

## 4. 判定

- Governed systemd service installation：Pass
- Installed `current` release as formal target startup path：Pass
- Headless OBC runtime under systemd：Pass
- OS reboot followed by autostarted installed stack：Pass
- Boot/update metadata still observable after reboot：Pass
- `.local` mDNS stability immediately after reboot：環境觀察，helper 已提供 `RPI_SSH_TARGET` 覆寫作為等價可重現路徑
- Full Raspberry Pi bootloader / partition handoff validation：Out of scope
