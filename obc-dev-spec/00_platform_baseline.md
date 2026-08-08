# 00 — 平台基線與文件分層

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/platform-baseline/spec.md`](../openspec/specs/platform-baseline/spec.md)。

## 1. 文件角色

本文件定義目前專案的共同背景、平台邊界、文件分層、啟動基線與後續 F' 專案落地方式。當 narrative 與正式主規格有衝突時，以 OpenSpec 主規格為準。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/00_dev_spec_main.md` | 專案目標、模式、架構總覽、命名與一致性原則 |
| `archive/legacy-v0/01_project_setup.md` | profile、目錄責任、F' bootstrap、ZMQ / GDS 基線 |
| `archive/legacy-v0/start-gds.md` | ZMQ proxy、simulator、deployment、GDS 啟動順序 |

## 3. 文件分層與唯一事實來源

| 層級 | 位置 | 角色 |
|---|---|---|
| Narrative source | `obc-dev-spec/` | 需求來源、設計脈絡、能力邊界與人工可讀說明 |
| Formal baseline | `openspec/specs/` | 目前系統應滿足的正式規格 |
| Change workspace | `openspec/changes/<change>/` | 每次變更的 proposal / specs / design / tasks 與實作過程 |

規則如下：

1. `obc-dev-spec/` 保留人類可讀的敘述層，但不再獨立長出與主規格不同的規則。
2. `openspec/specs/` 是正式基準；後續正式行為變更必須透過 OpenSpec change 更新並 archive。
3. 本次 `formalize-obc-baseline` 是唯一允許的 docs / governance-only baseline 變更；後續 change 必須同時包含規格、實作與驗證。

## 4. 專案目標與排除項目

第一版目標如下：

1. 建立可套用於 F' v4.1.0 的 CubeSat OBC Flight Software 正式規格基線。
2. 以 libcsp 建立 OBC 與 EPS / ADCS 模擬器之間的內部通訊，hosted profile 可用官方 ZMQHUB-backed 介面作為開發期承載層。
3. 建立外部通訊鏈路的 profile 切換策略，開發期使用 TCP mock，目標整合期可切換 UART。
4. 建立 Raspberry Pi 3B+ / Linux 導向的 Boot / Update A/B 更新與回滾規格。
5. 建立可重複執行的 OpenSpec 規格驅動開發循環。

以下內容不納入第一版正式基線：

- STM32 / MCU raw flash address、SRAM bank、DTCM、linker script
- 已定案的 CAN flight bus 細節
- 已定案的實體 radio vendor protocol 細節
- 需要實體線材或真實硬體才能完成的唯一驗證路徑

## 5. 執行模式與 profile

| profile | 目的 | OBC 運行位置 | 內部通訊 | 外部鏈路 |
|---|---|---|---|---|
| `dev-macos` | 純軟體開發與整合驗證 | macOS | libcsp over official ZMQHUB-backed hosted transport | TCP mock |
| `integ-rpi` | Raspberry Pi 3B+ 目標整合 | Raspberry Pi 3B+ | libcsp internal network | TCP mock 或 `/dev/serial0` external comm UART |
| `flight-hw-future` | 未來硬體 profile | Raspberry Pi 3B+ | libcsp 或後續 flight bus | UART / vendor adapter |

設計原則：

- 各 profile 共用相同的主邏輯。
- 差異僅能落在設定、instance 選擇、network endpoint 與 device path。
- 若某項差異需要複製主邏輯，必須先以 change 更新規格後才能實作。
- `integ-rpi` 必須同時保留 interactive SSH 啟動路徑與受管的 service-managed headless 啟動路徑；後者以 install root 的 `current` release 為唯一正式開機入口。

近程 multi-host 拓樸命名固定如下：

- `macOS`：ground host
- `obc` / `obc.local`：OBC target host
- `subsystem-sim` / `subsystem.local`：subsystem simulator host

目前所有 OBC-target helper scripts 的 canonical SSH target 名稱為 `OBC_SSH_TARGET`；舊的 `RPI_SSH_TARGET` 僅作 backward-compatible alias。`SUBSYSTEM_SIM_SSH_TARGET` 已保留給未來的 subsystem-side host workflow，但本階段尚未宣稱已具備完整 dual-Pi orchestration。

## 6. 平台與目錄基線

目前 repo 先保存文件與 OpenSpec 結構；F' 專案骨架會在後續 `bootstrap-fprime-platform` change 中落地。目標目錄基線如下：

```text
<project-root>/
├── CMakeLists.txt
├── settings.ini
├── fprime-venv/
├── OBC/
│   ├── Ports/
│   ├── Types/
│   ├── Components/
│   └── Deployment/
├── simulators/
├── scripts/
├── docs/
├── .github/
├── .codex/
└── openspec/
```

責任邊界如下：

- `OBC/`：F' 元件、共用型別、Deployment topology 與 profile-specific instances
- `simulators/`：EPS / ADCS / foundation peers 等 hosted CSP nodes、hub tooling 與支援元件
- `docs/`：測試紀錄、驗證證據與交付文件
- `.codex/`：專案級 OpenSpec workflow skills
- `openspec/`：正式規格與變更工作區

## 7. 官方初始化與整合基線

正式流程如下：

1. 初始化 Git repo。
2. 執行 `openspec init --tools codex .` 建立專案級 OpenSpec 與 `.codex/skills/`。
3. 以 `fprime-bootstrap project --populate --path . --tag v4.1.0` 作為唯一正式 F' 建專案方式。

補充規則：

- `fprime-bootstrap` 只在 `bootstrap-fprime-platform` change 中實作，不在本次 baseline formalization 中直接落地。
- `fprime-util` 會在後續 bootstrap 所建立的 `fprime-venv/` 中取得。
- OpenSpec 的 Codex 全域 prompt 安裝若受權限限制失敗，不影響專案內 `.codex/skills/` 的正式納管。
- `integ-rpi` 目標建置流程可省略同步 `.git` metadata，但受管 bootstrap 必須在 host 端先取得 project / framework version，並將其帶入 target build，避免 version 顯示退回 framework fallback 字串。
- `integ-rpi` 也必須提供 repo-local package / install flow：由 Raspberry Pi 原生 Linux artifacts 組成可審查 bundle，安裝到固定 user-writable root，並以 `current` release pointer 啟動，而不是把 synced source workspace 當成 deployable artifact。
- 若 `integ-rpi` 進入受管啟動階段，正式 target 啟動路徑必須由 repo-local helper 安裝 systemd service，並從 install root 的 `current` release 以 headless 模式啟動；這不等同於宣稱已完成 bootloader 或 partition handoff。
- `integ-rpi` 的 libcsp integration base 回 mainline 前，必須用 release-readiness evidence 記錄 shared gate、OpenSpec validation、legacy direct-ZMQ checker，以及已完成的 internal CSP / EPS CSP / ADCS CSP / retirement slices。
- `integ-rpi` 目標驗證需分開記錄 internal CSP substrate 與 external comm UART；兩者可在同一個 target run 中驗證，但不得描述成同一條 transport。

## 8. 通訊與啟動基線

第一版通訊策略如下：

| 項目 | 第一版定義 |
|---|---|
| 內部子系統網路 | libcsp；hosted foundation 採官方 ZMQHUB-backed transport；在 active `TopCcsds` 部署內由 `CspRuntimeOwner` 作為唯一 intended runtime owner |
| 外部通訊鏈路（開發期） | `Drv::TcpClient` / TCP mock |
| 外部通訊鏈路（整合期） | `UartDriver` over explicit device path；目前 Raspberry Pi 預設為 comm-owned `/dev/serial0` |
| 預設 ZMQ subscribe port | `6100` |
| 預設 ZMQ publish port | `7100` |
| 預設 GDS GUI port | `8080` |

建議啟動順序：

1. 啟動 internal CSP hub / proxy
2. 啟動 EPS / ADCS / foundation peer 等 hosted CSP nodes
3. 啟動 OBC deployment
4. 視需要啟動 GDS

基線驗證至少需確認：

- GDS 可開啟且可載入 deployment dictionary
- 基本 telemetry 可見
- `CSP_INIT` 成功
- `CSP_PING` 可打到 hosted CSP peer
- GDS 設定與 internal CSP hub 設定不混用

## 9. 文件索引

| 編號 | 文件 | 主題 |
|---|---|---|
| 00 | `00_platform_baseline.md` | 平台基線、文件分層、初始化與啟動順序 |
| 01 | `01_core_system_contracts.md` | 共享型別、Base ID、核心契約與 ownership 邊界 |
| 02 | `02_resource_storage.md` | 記憶體、buffer、儲存與 metadata 規劃 |
| 03 | `03_eps_subsystem.md` | EPS simulator / `EpsBridge` 契約 |
| 04 | `04_adcs_subsystem.md` | ADCS simulator / `AdcsBridge` 契約 |
| 05 | `05_comm_subsystem.md` | 外部通訊、`CommController`、`UartDriver`、`RadioController` |
| 06 | `06_boot_update.md` | Boot / Update A/B 策略 |
| 07 | `07_verification_evidence.md` | 測試分層、證據、紀錄模板與關鍵情境 |
| 08 | `08_delivery_workflow.md` | Git / PR / CI / OpenSpec 交付流程與 change queue |

## 10. 驗收錨點

本文件完成後必須滿足：

1. 文件分層清楚，且不再引用不存在的舊附錄。
2. profile 與運行模式清楚，且主邏輯不得依 profile 複製。
3. F' bootstrap、OpenSpec、ZMQ / GDS 基線可追溯。
4. narrative source 與正式主規格的角色分界明確。
