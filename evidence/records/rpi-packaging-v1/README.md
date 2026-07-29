# 測試紀錄：rpi-packaging-v1

- 日期：2026-03-30
- 層級：L3 / L4
- 環境：
  - host: macOS development machine
  - target: Raspberry Pi 3 Model B+ via `ssh operator@youjun.local`
- 關聯變更：`openspec/changes/rpi-packaging-v1/`
- 關聯文件：
  - `openspec/changes/rpi-packaging-v1/specs/platform-baseline/spec.md`
  - `openspec/changes/rpi-packaging-v1/specs/resource-storage/spec.md`
  - `openspec/changes/rpi-packaging-v1/specs/verification-evidence/spec.md`

> Historical note (2026-05-27): this record predates the current host-role naming.
> Any `operator@youjun.local` reference below is historical evidence only.
> Current governed OBC target references are `operator@obc.local` or `operator@<private-lab-host>`.

## 1. 目標

驗證 governed `integ-rpi` 流程已不只依賴 synced source workspace，也能：

- 從 Raspberry Pi 原生 Linux build 產物建立可審查的 install bundle
- 將 bundle 安裝到固定 user-writable install root
- 由 install root 的 `current` release 啟動 integrated stack
- 將 mutable runtime state 保持在 release payload 之外
- 在 `quit` 後乾淨退出，不殘留安裝版 simulator / OBC 程序

## 2. 執行指令

### 2.1 建立 Raspberry Pi bundle

```bash
bash scripts/package_rpi_bundle.sh
```

本次 bundle 產物：

- tarball: `build-artifacts/packages/rpi/0ab0a46-dirty/obc-rpi-0ab0a46-dirty.tar.gz`
- manifest: `build-artifacts/packages/rpi/0ab0a46-dirty/manifest.json`

manifest 摘要：

```text
release_id=0ab0a46-dirty
project_version=0ab0a46-dirty
framework_version=v4.1.0
bin/OBC 1626536 970e584ef4f0fc127a3ada12b15bcfb3888a308b964ff055cb90751945942b3f
bin/adcs_simulator 86680 27f26f9e6a1ecfb040a2c73b914ac0e4f4bb2452446b43e8a5b352714896c5b3
bin/eps_simulator 86192 65cb74b54442f8f02f5bd9fa1f1d7206cd7ca34289ad6a6f055ed13da54d0509
bin/radio_mock_server 76048 98eb68bf5447727476b91f6aeec1573b2f51b4e37649c3e715550a9554a0525d
dict/AppTopologyDictionary.json 119651 e753e3bb9be8da5e0ef24d7eb856071ad0bd48f70a12f8d895fc3d8ad60b37b8
launch/run_stack.sh 1383 0fc452d19e684159e71e085411a2c63920e62e400a3e195aea237f64bb235ba9
meta/version.json 91 410c875d31a316f884c4942242cb1d804160c7c9e34da4a3145e61ffe7699025
```

tarball 內容：

```text
./bin/OBC
./bin/adcs_simulator
./bin/eps_simulator
./bin/radio_mock_server
./dict/AppTopologyDictionary.json
./launch/run_stack.sh
./manifest.json
./meta/version.json
```

### 2.2 安裝到固定 install root

```bash
env FORCE_INSTALL=1 bash scripts/install_rpi_bundle.sh build-artifacts/packages/rpi/0ab0a46-dirty/obc-rpi-0ab0a46-dirty.tar.gz
```

Pi 上 install root 結構摘要：

```text
current=$OBC_HOME/obc-deploy/releases/0ab0a46-dirty
runtime=$OBC_HOME/obc-deploy/runtime/integ-rpi
$OBC_HOME/obc-deploy/current
$OBC_HOME/obc-deploy/releases
$OBC_HOME/obc-deploy/releases/0ab0a46-dirty
$OBC_HOME/obc-deploy/runtime
$OBC_HOME/obc-deploy/runtime/integ-rpi
$OBC_HOME/obc-deploy/runtime/integ-rpi/logs
$OBC_HOME/obc-deploy/runtime/integ-rpi/persistent-data
$OBC_HOME/obc-deploy/runtime/integ-rpi/staging
```

觀察結果：

- install root 為固定 user-writable path `$OBC_HOME/obc-deploy`
- immutable payload 位於 `releases/0ab0a46-dirty`
- `current` 指向該 release
- `persistent-data`、`staging`、`logs` 位於 shared runtime tree，而非 release 目錄內

### 2.3 從 installed release 啟動 stack

```bash
bash scripts/run_rpi_installed_probe.sh
```

關鍵觀察：

```text
Framework Version: [v4.1.0]
Project Version: [0ab0a46-dirty]
Storage roots: persistent=$OBC_HOME/obc-deploy/runtime/integ-rpi/persistent-data staging=$OBC_HOME/obc-deploy/runtime/integ-rpi/staging
mode=SAFE uptime=0 rebootCount=1
boot active=SLOT_A pending=NONE confirmed=yes progress=0 confirmTimeoutRemaining=0
radioLink connected=yes tx=14 rx=120 txErr=0 rxErr=0
```

probe 後再檢查：

```bash
ssh operator@youjun.local "ps -ef | grep -E 'obc-deploy/(current|releases/.+)/bin/(eps_simulator|adcs_simulator|radio_mock_server|OBC)' | grep -v grep || true"
```

結果為空，表示 installed launcher 在 `quit` 後沒有留下殘留程序。

## 3. 觀察與調整

- 初版 installed launcher 以 `exec` 直接取代 shell，導致原本用於清除 background simulators 的 `EXIT` trap 不會執行；本 change 已改為保留 parent shell，讓 `quit` 後可正常回收 companion processes。
- 初版 bundle tarball 在 macOS 打包後，Pi 解包時會出現 `LIBARCHIVE.xattr.com.apple.provenance` 警告；本 change 已改用 `tar --format ustar` 產生乾淨 tarball，後續安裝不再出現該警告。
- 本次驗證的 installed stack 仍沿用目前的 EPS / ADCS / radio mock integration flow，不宣稱已完成真實 UART / radio hardware 驗證。

## 4. 判定

- Raspberry Pi install bundle creation：Pass
- Bundle manifest / packaged version metadata：Pass
- Fixed install root + `current` release pointer：Pass
- Installed stack launch from install root：Pass
- Installed stack clean exit without stale processes：Pass
- Remaining real UART / radio hardware validation：Blocked-HW
