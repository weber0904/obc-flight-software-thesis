## Context

目前受維護的 ground operator baseline 是 `manual_ops` family 疊在 hosted / target
manual dual-GDS surface 上，核心 authority 仍來自 `manual_secure_ops.py`、
`surface_owner.py`、`secure_link_auth_lib.py`、以及 A/B baseline managers。
現況可完成 secure auth、secure-v2 command、governed upload、`SEQ_*` 操作，
但主要入口仍是 CLI，且 dashboard/readback/demo surface 幾乎缺席。

這次設計是 cross-cutting change：它同時碰到 manual operator helper、local web
gateway、listener ownership、readback parsing、UI surface、negative packet demo，
而且必須保持 current command authority 不變。`docs/roadmap/mission-console-phase1-handoff.md`
是本變更的 companion reference，所有 baseline/authority/reuse decisions 都以該文件與
現行 specs/code 為準，不重新發明一條平行 ground stack。

## Goals / Non-Goals

**Goals:**
- 建立一個 repo-owned `Mission Console` Phase 1，提供 dashboard、ops、detailed
  readback、surface 狀態與 packet-lab demo。
- 讓 `manual_secure_ops.py` 可被 Gateway 直接 import 呼叫，同時保留 CLI。
- 以 Gateway 自己擁有的 `fprime-cli events/channels` listener 建立 live snapshot，
  不依賴 manual surface 是否已經開 passive listeners。
- 用單一 `OperationRequest` / `OperationResult` 與 job/history model 統一 auth、command、
  upload、sequence、readback 與 packet-lab 動作。
- 維持 current baseline authority：不攔截 stock GDS UI、不建立第二條 command plane、
  不繞過 `SequenceAdmissionController`。

**Non-Goals:**
- 不做 one-GDS aggregation。
- 不把 stock GDS command tab 改成正式 secure operator UI。
- 不做 mission scheduler、persistent pass DB、generic planner。
- 不做 broad fuzzing framework 或 flight-like security lab。
- 不把 target shared service lifecycle 收進 Phase 1 的 writable admin plane。

## Decisions

### 1. Flask + server-rendered HTML/JS，而不是 React/Vite

採用 `Flask + server-rendered HTML/JS`。理由是 repo 目前已有 Flask 與 Python
tooling，且 Phase 1 的風險在 authority 整合、listener ownership、readback parsing，
不是前端框架能力。這條路最短，可在同一個 Python runtime 內共用 manual operator code。

考慮過的替代方案：
- React/Vite：互動更自由，但會引入額外建置層與更多檔案，對 Phase 1 只增加風險。
- Gateway only：太弱，無法達成 thesis/demo 目標。

### 2. Mission Gateway 只調用 repo-owned helper，不直接成為新 command authority

Gateway 只包裝既有 `manual_secure_ops` action surface，所有 secure auth / command /
file / sequence 動作仍由既有 helper 與 secure protocol truth 執行。這讓 current
authority boundary 保持單一，不會在 UI 層偷偷長出第二套 policy。

考慮過的替代方案：
- 直接在 Gateway 重寫 secure auth / secure command send path：風險高，容易和
  `secure_link_auth_lib.py` 漂移。
- 攔截 stock GDS UI command：違反 baseline non-claims，也更難驗證。

### 3. `manual_secure_ops.py` 保留 CLI，但抽成 importable structured action layer

`manual_secure_ops.py` 會被重整為：
- pure-ish action helpers
- structured result objects
- thin CLI adapter

這可同時滿足：
- 現有 runbook 不破
- Gateway 不必 shell-out 抓 JSON stdout
- 測試可直接打 Python function

考慮過的替代方案：
- 保持純 CLI，Gateway 只 subprocess：可行但脆弱，不利測試、history、job model。

### 4. Listener ownership 放在 Mission Gateway，而不是 manual surface

current hosted manual surface 刻意關掉 passive listeners，因此不能假設現有 baseline
會提供穩定可讀的 `events.log` / `channels.log`。Gateway 必須自己啟動
`fprime-cli events/channels` listener，並在 surface root、ownerPid、tts port 變更時重啟。

考慮過的替代方案：
- 修改 manual surface 預設打開 passive listeners：會擴大既有 baseline 行為變更。
- 直接 tail stock GDS runtime logs：格式與 ownership 都不穩。

### 5. Readback 要明確區分 `event-based` 與 `channel-refresh-based`

repo 現有 readback contract 已分成：
- explicit `GET_*` 觸發 channel refresh
- explicit command 觸發 event readback

例如 `TTC_GET_STATUS` 主要刷新 `TTC_POLICY_*` telemetry，而
`GET_RECOVERY_STATUS` / `BOOT_STATUS` / `GET_PERSISTENT_FAULT_HISTORY`
主要靠 event。Gateway parser layer 必須顯式表達這兩類，不做單一 generic parser。

考慮過的替代方案：
- 全部當 event parse：會漏掉 `TTC_POLICY_*` 類型。
- 全部當 channel diff：會漏掉 recovery / payload metadata / fault history 類型。

### 6. Packet lab 是 bounded demo surface，不是 fuzzing framework

`packet-lab` 只做：
- replay captured raw
- replay stale session
- duplicate / tampered sequence
- tampered MAC

每個 case 都只顯示關鍵欄位與錯誤 highlight，不暴露完整封包解碼器 UI。這足以支撐
demo，也避免把 Phase 1 擴成 generic malformed packet workbench。

考慮過的替代方案：
- 完整 packet editor/fuzzer：超出 Phase 1。
- 完全不做 packet lab：無法直接支撐論文 demo 對 security contribution 的呈現。

### 7. Hosted first，再做 target parity

先在 hosted path 做完整閉環：registry、listeners、dashboard、ops、readback、
packet-lab。target 之後重用同一套 Gateway/UI，僅接上 target provenance/preflight
gate 與不同 manifest/baseline metadata。

考慮過的替代方案：
- hosted/target 同步：會把 integration matrix 一開始就放大。
- target first：不利快速形成穩定 demo surface。

## Risks / Trade-offs

- [Listener ownership 與 surface lifecycle 競爭] → Gateway 只擁有自己的 listeners，不接管 GDS/gateway runtime；用 ownerPid/tts-port drift 檢測自動重啟。
- [Readback parser 漏掉現有 command-specific semantics] → 以 curated command map 實作，先覆蓋 plan 中列出的正式 commands，不做假泛化。
- [CLI refactor 破壞既有 runbook] → 保留 argparse surface 與 JSON stdout；新增單元測試保護 CLI parity。
- [Packet lab 缺少明確 reject signal] → 規定 bounded negative evidence classifier，可接受 `no completion`、`sequence not advanced`、`state unchanged` 作為顯式 fallback。
- [Target path 太早引入 shared service lifecycle 寫操作] → Phase 1 的 `/surfaces` 僅 read-only，不新增 target shared service admin button。
- [Mission Console 自己的狀態檔與 evidence 漂移] → `action-history.jsonl`、`packet-lab-history.jsonl`、listener logs 都放在 Mission Console root，並明確標成 diagnostic/ground-owned，不混充 flight truth。

## Migration Plan

1. 建立 OpenSpec change 與實作 branch。
2. 重整 `manual_secure_ops.py` 成 importable action layer，保留 CLI。
3. 新增 `scripts/mission_console/` Flask skeleton、registry、job model。
4. 新增 listener ownership、snapshot cache、readback parser。
5. 新增 dashboard / ops / readback / surfaces / packet-lab UI。
6. 先在 hosted path 驗證閉環，再接 target parity。
7. 補齊 docs 與 tests，執行 relevant verification。

回滾策略：
- 若 Mission Console 發生問題，可完全停用新 app，既有 hosted/target manual dual-GDS
  surface 與 `manual_secure_ops.py` CLI 仍可獨立使用。
- 不以 Mission Console 取代 stock GDS 或既有 manual helper，因此回滾不需要改 flight
  或 COMM baseline。

## Open Questions

- 無。Phase 1 的 UI、authority、listener、readback、packet-lab 邊界已由本變更計畫固定。
