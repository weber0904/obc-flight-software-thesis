## Why

Mission Console 目前雖然已經把 command、readback、packet-lab 與 dashboard
收斂成結構化頁面，但 operator-facing UI 仍缺一塊很重要的 pass-time
觀測能力：背景持續下傳的 ADCS / EPS / health 值還沒有被做成可用的連續圖表，
而現有 dashboard 排版與欄位選擇也還混有 mode-safety 這類 breadcrumb 欄位，
不夠直觀。另一方面，現有 `/readback` 頁仍把 `GET_*` 執行入口、讀回結果
保存、以及 proof/debug 證據混在同一個操作面，對實際 operator 使用並不實用。
此外，Mission Console 目前也只有「上傳既有 `.bin` 然後執行 `SEQ_*`」，
還缺少一個正式的 sequence 檔案建立頁面；operator 若要做新的 sequence，
仍得回到 CLI 手寫 `.seq` 並自行呼叫官方 `fprime-seqgen`，這和目前 repo
已經收斂好的 governed sequence upload / validate / run flow 是斷開的。

現在正是把這一層補齊的時機，因為 shared-channel refresh 與 event/report
readback closing 已經收斂完成，UI 可以直接站在現有 snapshot truth 上做
operator-facing 提升，而不用再邊做畫面邊回頭猜 flight-side 語意。

## What Changes

- 新增 Mission Console 的 live trend 工作區，讓 operator 可依次系統勾選
  持續下傳欄位，並在同一張圖上疊多條曲線觀測；第一版只含 `EPS`、
  `ADCS`、`Health`，不把 `GPS` 當作第一版 active trend group。
- 為 Mission Console gateway 補上 bounded channel history cache，支援
  live trend 讀取，而不把 chart 建立在 raw log tail 或無界資料累積上。
- 重排 dashboard 版面，降低目前 `Comm State` / `Mode Overview` 的混亂度，
  補上更合理的 `EPS` / `ADCS` operator-facing 內容。
- `dashboard` 維持純狀態頁，但補上 selected context/band `Recent Events` ring
  的 clear control，讓 operator 可清空當前觀測視窗而不影響 saved readback。
- `dashboard` 另外補上 `Clear context cache`，讓 operator 可快速清掉目前 context 的
  觀測快取並重新判斷哪些欄位仍缺值，不必手動重啟整個 console。
- 為 `dashboard / trends / readback viewer` 補上明確的 repo-owned 視覺語言，
  讓它更像 mission-control data canvas，而不是預設工程表單或 stock GDS debug panel。
- 重構 `Ops` / `Readback` 分工：
  - `Ops` 保留 `GET_*` 在內的指令送出入口
  - `Readback` 改成保存後的 viewer，單一路由內以 tabs 呈現最近一次保存的
    subsystem / family 結果
  - `Readback` 每張 saved card 可直接用小型 refresh button 重送它對應的
    `GET_* / status` command，不必切回 `Ops`
  - 將現有 fresh-evidence / fallback / incomplete / reject 類 proof 細節降到
    次層或獨立 debug 視圖
  - `Ops` 的 `Job Result` 只保留 compact dispatch summary；對 `GET_* / status`
    類指令，不再把整份 readback payload 塞回 ops 頁主閱讀面
  - `Dashboard` 保持純狀態檢視頁，不新增任何 refresh / dispatch control
- 補上跨頁 `context / band` selector 記憶，讓 operator 在不同頁面間切換時可保留：
  - 上次使用的 context
  - 每個 context 上次使用的 band
- 將主 accent 與按鈕/active-chip 視覺從過亮的綠色收斂成較中性的深藍灰簡約風格，
  同時保留狀態 badge 的語意色。
- 將 dashboard channel tiles 內過長的 `Background update / Flight sampled / Gateway observed / Via ...`
  metadata 收成 hover tooltip，主畫面只保留值與欄位名稱，降低卡片內文雜訊。
- 將 dashboard 改成混合資料模型：
  - `Satellite Status`、`Comm State`、`EPS Snapshot`、`ADCS Snapshot`、
    `Health Snapshot` 取同一 context 內各 band 最新觀測值
  - `Secure Session` 保持 selected-band truth
  - 回傳值保留 `sourceBand`，讓 UI 能說明這筆值最近是經由哪個 band 看見的
  - dashboard 主頁要直接用 `Via <band>` 呈現這個來源，而不是只把 band 當隱含 backend 細節
- 同一個 `GET_*` / status command 不論是從一般 command workspace 或 raw fallback 送出，只要 command name 屬 readback family，就必須保存到同一套 `/readback` viewer
- `Readback` 要明確分離：
  - 成功關閉所需的最小 fresh evidence
  - operator viewer 想看的完整保存欄位
  不能再因為 closing contract 很小，就讓 viewer 只剩最小證據欄位
- 把 `/trends` 從單大圖改成多 panel 工作區：
  - 預設兩張 panel
  - 可追加到最多四張
  - 每張 panel 各自選 subsystem 與 series
  - 每張 panel 用自己的 Y 軸 fitting，不再共用右側大型 legend / notes 側欄
- 將 `Action History` 與 `Packet Lab History` 改成 console-session-first：
  - 預設只顯示本次 Mission Console session
  - 支援 `show all`
  - 支援 `clear current session`
  - 歷史容器固定高度可捲動
- 新增 Mission Console 的 governed sequence authoring workspace：
  - 以官方 `.seq` 語法為輸出真相
  - 支援結構化 sequence step 編輯
  - 透過 active context dictionary 呼叫官方 `fprime-seqgen`
  - 重用既有 governed upload 與 `SequenceAdmissionController` `SEQ_*` surface
  - drafts、generated `.seq`、compile 產物保存在 `MISSION_CONSOLE_ROOT`，
    不寫進 repo tree
- 重做 `packet-lab` 的 demo 呈現：
  - 明確區分地面端「故意注入的錯誤模型」與 flight 端「實際觀測到的拒絕」
  - 直接顯示封包中故意做錯的關鍵欄位
  - 顯示應有值 / 實際值 / 錯誤點
  - 將 reject reason number 轉成可讀錯誤名稱
- 將 `MODE_SAFETY_LAST_CURRENT_MODE`、
  `MODE_SAFETY_LAST_TARGET_MODE` 等 breadcrumb 型 autonomy 欄位降到次要或
  diagnostics 呈現，不再放在 dashboard 前排主操作摘要。
- 收斂 Mission Console trend/dashboard 預設只使用 operator-facing continuous
  truth，不把 diagnostics-only residual chatter 直接提升成預設可視圖表面。
- 補一輪 residual live-chatter inventory，確認現行仍持續下傳的 channel 中，
  哪些只是文件降級但 runtime 仍在送，作為後續 flight-side cleanup 依據。

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `mission-console`: 增加 live trend 工作區、dashboard 版面重排、以及更清楚的
  operator-facing summary/trend/readback/packet-lab 呈現規則與視覺層級，並新增
  governed sequence authoring / compile / upload / validate workflow。
- `mission-console-observability-bootstrap`: 補充 continuous trend 預設來源與
  diagnostics-only residual chatter 的 UI 使用邊界，避免 trend/dashboard
  誤把非 operator truth 的 channel 升成主視圖。

## Impact

- `scripts/mission_console/app.py`
- `scripts/mission_console/gateway/snapshots.py`
- `scripts/mission_console/gateway/parsers.py`
- `scripts/mission_console/gateway/catalog.py`
- `scripts/mission_console/gateway/sequence_authoring.py`
- `scripts/mission_console/templates/*.html`
- `scripts/mission_console/static/mission-console.js`
- `scripts/mission_console/static/mission-console.css`
- `scripts/test_mission_console_phase1.py`
- `scripts/mission_console/probe_client.py`
- `openspec/specs/mission-console/spec.md`
- `openspec/specs/mission-console-observability-bootstrap/spec.md`
- `docs/roadmap/mission-console-phase1-handoff.md`
- `docs/roadmap/mission-console-observability-recommendations.md`
- `docs/test-records/mission-console-phase1/README.md`
