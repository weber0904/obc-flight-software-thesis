# 測試紀錄：target-version-metadata-v1

- 日期：2026-03-30
- 層級：L3
- 環境：
  - host: macOS development machine
  - target: Raspberry Pi 3 Model B+ via `ssh operator@youjun.local`
- 關聯變更：`openspec/changes/target-version-metadata-v1/`
- 關聯文件：
  - `openspec/changes/target-version-metadata-v1/specs/platform-baseline/spec.md`
  - `openspec/changes/target-version-metadata-v1/specs/verification-evidence/spec.md`
  - `docs/interfaces.md`

> Historical note (2026-05-27): this record predates the current host-role naming.
> Any `operator@youjun.local` reference below is historical evidence only.
> Current governed OBC target references are `operator@obc.local` or `operator@<private-lab-host>`.

## 1. 目標

驗證在不同步 `.git` 到 Raspberry Pi 的前提下，受管 `integ-rpi` bootstrap 仍會產生正確的 framework / project version metadata，而不是退回 framework fallback `v3.5.0`。

## 2. 預期版本來源

Host 上先確認這次 build 應帶入的版本：

```bash
git describe --tags --always --dirty --broken
git -C lib/fprime describe --tags --always --dirty --broken
```

觀察結果：

```text
project_version=0ab0a46-dirty
framework_version=v4.1.0
```

## 3. 執行指令

### 3.1 Governed target bootstrap

```bash
bash scripts/bootstrap_rpi_workspace.sh
```

關鍵觀察：

- host workspace 仍維持 `.git` 為版本來源
- 同步到 Pi 的 workspace 不包含 `.git`
- Pi 上 `fprime-util generate -f` 與 `fprime-util build` 成功完成

### 3.2 檢查 Pi 上生成的版本檔

```bash
ssh operator@youjun.local 'cat $OBC_HOME/lab/fprime/v0/build-fprime-automatic-native/versions/version.json'
ssh operator@youjun.local 'cat $OBC_HOME/lab/fprime/v0/build-artifacts/Linux/OBC/dict/AppTopologyDictionary.json | head -n 5'
```

觀察結果：

`versions/version.json`

```json
{"framework_version": "v4.1.0", "project_version": "0ab0a46-dirty", "library_versions": {}}
```

dictionary metadata 摘要

```json
{
  "metadata" : {
    "deploymentName" : "OBCApp.App",
    "projectVersion" : "0ab0a46-dirty",
    "frameworkVersion" : "v4.1.0",
```

## 4. 結論

- Pi target build 已不再退回 `v3.5.0`
- 生成的 framework / project version 與 host 端 `git describe` 一致
- 修正方式維持在 repo-local project layer，未直接修改 `lib/fprime` 子模組內容

## 5. 限制

- 本次修正保證的是 governed `scripts/bootstrap_rpi_workspace.sh` 路徑
- 若在 Pi 上跳過該 bootstrap、直接手動重新 configure/build，則可能不會帶入相同的 host-derived version metadata
