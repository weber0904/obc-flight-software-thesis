## Context

`Mission Console` 目前已完成 command workspace、structured readback、packet-lab
與 surfaces 頁面，且 observability bootstrap 已把 shared-channel refresh、
single-event readback、multi-event readback 的 flight-side 與 closing semantics
收斂完成。

但目前 UI 仍有三個明顯缺口：

1. 連續下傳值只有 latest snapshot，沒有可直接操作的連續圖表。
2. dashboard 版面仍混雜 operator 真值與 autonomy breadcrumb，
   `Comm State` / `Mode Overview` 的視覺權重失衡。
3. `EPS` / `ADCS` 雖已有正式 operator-facing 欄位，卻沒有被整理成清楚的
   operator summary 與 trend 入口。
4. `/readback` 目前仍是 proof/debug 導向頁：
   - 在同一頁直接送 `GET_*`
   - 只顯示單次 job 結果
   - 將 `family`、`fresh evidence`、`fallback`、`incomplete`、`reject`
     等證據細節直接暴露在主閱讀面
   這不利於操作者長時間查看已保存的 subsystem readback。
5. governed sequence flow 目前只有：
   - 上傳既有 `.bin`
   - 執行 `SEQ_VALIDATE / SEQ_RUN / SEQ_PREPARE_MANUAL / ...`
   但沒有 repo-owned sequence 建立頁。operator 若要新增 sequence，仍得回
   CLI 手寫 `.seq` 並自行呼叫官方 `fprime-seqgen`，這和 Mission Console 其餘
   operator workflow 已經收斂成頁面的方向不一致。

這次 change 的目的不是重做 flight-side observability contract，而是在
既有 contract 之上做 operator-facing UI 收斂，並把 trend 所需的 bounded
history cache 補齊。

約束如下：

- 維持 `Flask + server-rendered HTML + vanilla JS`
- 不引入 React/Vite
- 不改既有 secure/operator authority
- 不把 diagnostics-only residual chatter 直接提升成主操作圖表面
- 先做 repo-owned Mission Console，不耦合 stock `fprime-gds` addon 路徑
- UI 必須帶有設計節制與 demo 可用的美感，不能退回預設工程表單牆

## Goals / Non-Goals

**Goals:**

- 新增 Mission Console live trend 工作區，支援依次系統勾選欄位並在同一張圖上
  觀測多條曲線。
- 為 trend 頁面補上 bounded per-channel history cache，不依賴 raw log tail 或
  無界前端累積。
- 重排 dashboard，讓 operator 真值、`EPS` / `ADCS` 最新狀態、secure session、
  health 與 comm posture 更清楚。
- 重構 `Ops` 與 `Readback` 的角色分工：
  - `Ops` 負責送出 `GET_*`
  - `Readback` 負責查看保存後的最新 readback
  - proof/debug 資訊退到次層或獨立 debug 視圖
- 新增 governed sequence authoring workspace：
  - 以官方 `.seq` 文字格式作為最終真相
  - 用結構化 step 編輯器輔助產生 `.seq`
  - 用 active context dictionary 呼叫官方 `fprime-seqgen`
  - 重用既有 governed upload 與 `SequenceAdmissionController` `SEQ_*`
- 把 `MODE_SAFETY_LAST_CURRENT_MODE`、
  `MODE_SAFETY_LAST_TARGET_MODE` 這類 breadcrumb 從 dashboard 主摘要降級。
- 明確界定 trend/dashboard 預設只吃哪些 continuous / operator-facing channel。
- 補一輪 residual live chatter inventory，確認哪些 channel 雖已文件降級，但
  runtime 仍在持續送出。
- 為 `dashboard / trends / readback viewer` 建立一致的 mission-control 視覺語言，
  讓其適合口試 demo，而不只是 proof/debug 容器。
- 讓 `packet-lab` 成為可直接 demo 的錯誤封包說明面，而不是只顯示「送了某個錯誤案例然後被拒絕」。

**Non-Goals:**

- 不重開 `MODE / EPS / ADCS / GPS / TTC / STORAGE / COMM / BOOT` 的
  flight-side observability 分類討論。
- 不在這一輪直接做全面 flight-side residual telemetry stop-the-world cleanup。
- 不在這一輪重做 `surfaces` 的完整 UX。
- 不引入新的 Mission Console authority、WebSocket、SSE、資料庫或外部前端框架。
- 不承諾 target parity 的新 UI 自動化；仍以 hosted operator proof 為主。
- 不直接照抄外部產品 UI；外部設計稿只作原則提取，不作一比一仿製。

## Decisions

### 1. 新增獨立 `/trends` 頁，而不是把完整圖表塞進 dashboard

dashboard 的角色應維持在「快速看 posture 與最新值」，不適合直接承擔大量
多曲線互動圖表。這次因此新增獨立的 `/trends` 頁：

- 上方保留 context / band selector
- 提供 subsystem 分群：
  - `EPS`
  - `ADCS`
  - `Health`
- 每群用 checkbox 控制是否顯示某個 channel
- 同一張圖允許多條曲線同時疊圖
- 第一版只開 curated operator-facing continuous channel，不直接 expose
  dictionary 全量 channel

替代方案：

- 把大圖直接做進 dashboard：
  - 缺點是主畫面會再次變亂，且 chart 會壓縮 posture 資訊。
- 沿用 stock `fprime-gds` chart：
  - 缺點是無法與 Mission Console 的 context/band/snapshot truth 整合，也會把
    operator UX 繼續分散在兩套介面。

### 2. trend 歷史使用 gateway-owned bounded history cache

Mission Console 目前只有 latest snapshot，沒有 time-series history。
這次新增 gateway-owned bounded per-channel history：

- 以 `(contextId, band, channel)` 為 key
- append 新 sample
- 保留固定數量或固定時間窗
- API 只回指定 channel 的 bounded window

第一版可採 append+prune 的 bounded list/ring-buffer-like 實作，不需要先引入
外部資料庫或複雜 TSDB。

替代方案：

- 前端自行從 polling 開始後累積：
  - 重新整理就失憶，且無法共享不同頁面與 job refresh 之間的時間序列。
- 直接讀 listener 原始 log：
  - 解析成本高、來源混雜、無法乾淨對齊 current snapshot truth。
- 直接依賴 stock `fprime-gds` datastore：
  - 會讓 Mission Console 對 venv 內前端實作產生不必要耦合。

### 3. chart 實作採 repo-owned path，而不是直接引用 stock GDS addon

stock `fprime-gds` chart 目前也是 bounded in-memory history；這可作為設計參考，
但 Mission Console 不應直接依賴 `fprime-venv` 內的 addon 路徑。

因此這次採：

- Mission Console 自己維護 repo-owned chart 實作
- 使用簡單可控的 multi-series line chart 模式
- 不先引入 stock GDS 那套 full addon wrapper

理由：

- 可避免 Python venv / upstream static path 變動影響 repo-owned UI
- 可以只做需要的 operator features，不把 stock GDS 全套 chart UX 拖進來

### 3a. 視覺語言採「節制的產品化介面」，但保留 mission-control 識別

外部設計稿可借用的不是完整視覺外觀，而是以下原則：

- 用 spacing 而不是大量分隔線做層級
- 用非常淡的陰影與表面深淺代替硬邊框
- 導覽、chip、toggle、button 保持簡潔克制
- 字級/字重/留白要形成穩定 hierarchy

但 Mission Console 不採完全 achromatic 的產品風格，理由是：

- `EPS / ADCS / Health` 需要能被快速視覺區分
- 趨勢圖需要 subsystem 色系，不適合只靠單一 accent 色
- 口試/demo 需要清楚辨識資料群組，而不是過度安靜的 SaaS 後台

因此這次的視覺方向固定為：

- 結構、間距、控制元件節制感可參考成熟產品化介面
- chart、subsystem、status 則保留 repo-owned mission-control 配色與識別
- 不直接複製 stock GDS 或外部網站的具體視覺

### 3b. `context / band` selector 必須跨頁記憶

目前 Mission Console 的頁面切換頻率很高：

- `/ops` 送指令
- `/readback` 看保存結果
- `/trends` 看連續變化
- `/packet-lab` 做 demo

若每次切頁都把 band 重設回 `sband`，操作者在 `uhf` 路徑上會一直重選，操作體驗
很差。

這次固定做法：

- 前端保存：
  - `lastContextId`
  - `perContextLastBand[contextId]`
- 所有主要頁面共用同一套 hydration 規則
- 若某 context 目前沒有該 band，才 fallback 到該 context 第一個可用 band

這個狀態是 operator UX 層，不改底層 snapshot store 的 context/band 真值模型。

### 4. dashboard 改成 context-latest posture-first + band-scoped secure session

dashboard 的主要摘要不是 readback proof 面，因此不應因為 UI 當前切到某個 band，
就把同一 context 下其實已有最新值的 posture card 清成 `Unavailable`。

目前 `Mode Overview` 混入：

- `SYS_MODE`
- `MODE_SAFETY_LAST_CURRENT_MODE`
- `MODE_SAFETY_LAST_TARGET_MODE`
- `ADCS_MODE`
- `GPS_SOURCE_MODE`
- `EPS_PDU_STATUS`
- `PAYLOAD_STATE`

這會把「目前衛星真實狀態」和「mode safety 上一次判斷 breadcrumb」混成同層。

這次改成：

- 主摘要只留 operator 真值
- `ModeSafety*` 降級到 autonomy detail / secondary panel
- dashboard 另補：
  - `EPS Snapshot`
    - `EPS_VBAT`
    - `EPS_IBAT`
    - `EPS_SOC`
    - `EPS_TEMP_BAT`
    - `EPS_PDU_STATUS`
  - `ADCS Snapshot`
    - `ADCS_MODE`
    - `ADCS_OMEGA_X`
    - `ADCS_OMEGA_Y`
    - `ADCS_OMEGA_Z`
  - `Satellite Status`
    - `SYS_MODE`
    - `ADCS_MODE`
    - `EPS_PDU_STATUS`
    - `GPS_SOURCE_MODE`
    - `PAYLOAD_STATE`
  - 既有 `Comm State`、`Secure Session`、`Health Snapshot`、`Sequence & Ops`
    重新排版但不改 authority semantics

另外固定加上：

- `Satellite Status`、`Comm State`、`EPS Snapshot`、`ADCS Snapshot`、
  `Health Snapshot`
  - 從同一 context 各 band snapshot 中挑最新 `observationAt`
  - 回傳時保留 `sourceBand`
  - UI 必須把它顯示成欄位次要文字中的 `Via <band>`，避免操作者誤以為整張 dashboard 完全只屬當前 selected band
- `Secure Session`
  - 保持 selected-band truth
- `recent events`
  - 仍維持 selected-band recent ring buffer，不混成 context-wide chronology
  - 允許由 dashboard 直接清空當前 selected context/band 的 gateway ring
  - clear 只影響目前事件視窗，不影響 saved readback 與 flight 端狀態
- `context cache clear`
  - dashboard 另提供 `Clear context cache`
  - 清目前 selected context 的 channel snapshots、recent-event rings、saved readback cache、trend history、last-action cache
  - 不清 action history / packet-lab history / secure-state / flight 端狀態
  - 目的只是讓 operator 能快速回到「目前還沒重新觀測到哪些值」的空白基線

替代方案：

- 保留現有欄位，只改 CSS：
  - 不能解決資訊語意混亂。
- 把 `ModeSafety*` 直接刪掉：
  - 會失去有價值的 autonomy breadcrumb；更合理的是降級而不是抹除。

### 4a. history 預設只看本次 console session

目前 `Action History` 與 `Packet Lab History` 一直累積所有舊紀錄，會讓：

- demo 時很難看出這次操作剛做了什麼
- 歷史區塊無上限往下撐
- operator 很容易把昨天的紀錄誤認成這次剛得到的結果

因此這次固定改成：

- backend 每筆 action / packet-lab history 都帶 `consoleSessionId`
- 預設只顯示本次 Mission Console session
- 提供 `show all`
- 提供 `clear current session`
- 歷史區塊固定高度可捲動

這仍保留 cross-session review 能力，但不把它當預設操作模式。

### 4b. `/trends` 收斂成多 panel 工作區

單大圖加右側 permanent legend / notes，會導致：

- 圖表可用寬度被吃掉
- 不同量級 series 混成同一尺度時更難看
- 一次只想分開看 `EPS` 與 `ADCS` 時，反而更亂

這次固定改成：

- 預設兩張 panel
- 可用 `+ Add Panel` 追加，最多四張
- panel 垂直堆疊
- 每張 panel 可獨立：
  - 選 subsystem
  - 選 series
  - 決定是否在同 panel 疊多條線
- 每張 panel 只對自己選到的 series 做 Y 軸 fitting
- legend 直接整合在 panel 內
- `Chart Notes` 降級成小型 help/details，不再常駐佔右側版面

### 5. `/readback` 改成 viewer-first，proof/debug 退到次層

目前 `/readback` 的問題不是缺資料，而是資料面角色混用：

- 同一頁既是 `GET_*` 按鈕牆，又是結果顯示區
- 結果以單次 job 為中心，不是以 subsystem 最新保存值為中心
- `Channel Fields` 與 `Fresh Channel Evidence` 在
  `channel-refresh-based` 成功案例中高度重複
- `family`、`channelSource`、`eventSource`、`fallbackEvidence`、
  `incompleteResult`、`completionEvidence` 等 proof 診斷面資訊直接佔據主視覺

這次改成：

- `Ops`
  - 保留 `GET_*` 在內的操作入口
  - 若 command name 屬 readback family，無論是從一般 command workspace 或 raw fallback 送出，都必須落到同一套 saved readback cache
- `Readback`
  - 改成 viewer-first 頁面
  - 依 subsystem / 類別顯示最近一次保存成功的 readback
  - 顯示最後刷新時間、來源指令、成功/部分成功/失敗狀態
- `Readback Debug`
  - 保留 proof/debug 細節
  - 顯示 `family`、fresh evidence、fallback、incomplete、reject、
    source provenance 等資訊

第一版固定保留單一路由 `/readback`，並在該頁內以 tabs 呈現：

- `OBC`
- `EPS`
- `ADCS`
- `Payload`
- `Storage`
- `Boot & Recovery`
- `Sequence`

proof/debug 細節保留在同頁 secondary panel 或 `Proof / Debug Details`
展開層。operator 主畫面不得再以 proof 細節作為主要閱讀層。

這一頁的主語意固定為：

- `Latest Saved Values`
  - 看這個 family 最近一次保存下來的結構化真值
- `Last Refresh Evidence`
  - 只在 secondary proof/debug 層看這次是靠哪些 fresh channel / event 關閉
    成功

### 5d. `/readback` 提供 card-level quick refresh，但 `/dashboard` 不承擔 dispatch

雖然 `/ops` 已是正式 `GET_*` 入口，但實際 operator 操作時，若只是想在檢視
保存值的同時立刻再查一次，來回切頁會中斷閱讀節奏。因此這次補一個受限的
quick refresh：

- 只做在 `/readback`
- 只做在每張 saved readback card 的 header 右上角
- 只對有明確 `sourceCommand` 且屬 readback family 的 card 顯示
- 點擊後仍走既有 `/api/readback/run`、既有 job runner、既有 readback cache

這個功能的定位固定是：

- `Ops`
  - 正式操作與完整 dispatch 工作面
- `Readback`
  - 以 viewer 為主
  - 允許「就地重查同一張卡」的快捷操作

明確不做的事情：

- 不把 `/readback` 重新做回 button wall
- 不在每個 value tile 放 field-level refresh
- 不在 `/dashboard` 加 refresh icon 或隱藏 dispatch control

原因是 `/dashboard` 的責任仍然是 posture/latest status。若把 refresh 帶進
dashboard，就會重新混淆「摘要檢視」與「具名讀回」兩種頁面角色。

quick refresh 的 state model 固定為每張 card 各自獨立：

- `idle`
- `refreshing`
- `succeeded`
- `failed`

它只影響該 card 的 header 狀態與 compact error，不得把整頁 `/readback`
重畫成單一全域 job 結果面，也不得把結果導回 `/ops` 的 `Job Result`。

### 5e. 主 accent 收斂成深藍灰簡約風格，狀態語意色維持獨立

先前頁面雖然已經有較完整的資訊層級，但 primary buttons、active chips、
quick refresh controls 仍偏向高飽和綠色，會讓整體介面看起來像尚未收斂的工程 demo。

這次固定做法：

- 主 accent 改成較中性的深藍灰
- active navigation、selected chips、primary buttons、quick refresh icon 共用同一套 accent
- `good / warn / bad` 類 status badge 保持獨立語意色，不跟主 accent 混用
- 不引入第二套高彩度品牌色，也不把整頁做成過度花俏的 demo 風格

### 5f. dashboard channel metadata 改成 hover tooltip，而不是長行內文

實際驗收顯示 dashboard card 內若直接展開：

- `Background update`
- `Flight sampled ...`
- `Gateway observed ...`
- `Via <band>`

會讓 tile 內文高度不一致、頻繁換行，破壞主摘要頁的乾淨度。

因此 dashboard 的 channel tiles 改成：

- 主畫面只顯示欄位名與值
- metadata 收成 tooltip
- 在欄位名旁放極小的 info indicator，提示該 tile 有額外 provenance 可 hover 查看

這不改 dashboard 的 status-only 性質，因為它沒有新增 dispatch，只是把既有
provenance 從常駐文字改成按需查看。

第一輪實作曾把 `/ops` 的 `Job Result` 放到右欄上方並做成 sticky，但後續 UI 驗收顯示這會帶來新的版面問題：

- 結果 payload 一長，就把右欄卡片持續往下擠
- 滾動時形成懸浮覆蓋感
- `GET_*` 結果與 `/readback` viewer 重複呈現

因此最終做法固定改成：

- `Job Result` 移到 `Secure Command Workspace` 下方
- 保持靠近 primary command workspace，但不再是右欄 sticky sidebar
- 對 `GET_* / status` 類結果只顯示 compact summary：
  - job state
  - command
  - status
  - readback family
  - fresh channel/event counts
- 詳細 saved values 與 proof payload 一律回 `/readback`

### 5b. `Readback` viewer 要分離「最小成功證據」與「完整顯示欄位」

shared-channel refresh tranche 已經有明確的 closing semantics，但 viewer 若直接重用同一份最小 closing map，會出現明顯的 operator UX 問題：

- probe 關閉所需最小 fresh evidence 可能只有少數 channel
- 但 flight 端這次 refresh 實際上會送更多 operator-facing 欄位
- viewer 若只保存最小 closing 子集，就會讓操作者誤以為這次 `GET_*` 根本沒有回更多資料

像 `ADCS_GET_ATTITUDE` 就屬於這一類：自動化最小 closing contract 可以只看少數欄位，但 viewer 不能因此只剩那幾個欄位。

這次固定分成兩層：

- `minimum fresh evidence`
  - 只用於 pass/fail closing
- `viewer display fields`
  - 用於保存後的 `/readback` viewer 顯示

這樣能同時保留：

- repository-owned probe / test 的嚴格最小契約
- operator 在 `/readback` 實際想看的完整保存結果

### 5c. `Proof / Debug Details` 及其內層 raw payload 都必須保存展開狀態

`/readback` 目前以 polling 方式刷新。若只保存外層 `Proof / Debug Details` 狀態，而內層 `Raw Readback Payload` 每次 refresh 都自動收起，debug 面仍然不可用。

因此這次固定要求：

- `Proof / Debug Details` 展開狀態持久化
- `Raw Readback Payload` 這類內層 secondary details 也要持久化
- persistence key 以 context / band / command 為主，不要求跨不同 command 共用

替代方案：

- 保留現狀，只微調文案：
  - 不能解決主次資訊錯置與結果不保存的問題。
- 把 proof/debug 資訊整個刪掉：
  - 會破壞 observability bootstrap 既有可驗證性；應保留但降級。

### 5a. `dashboard / trends / readback viewer` 共用同一套視覺節奏

這一輪三個主要 operator 頁面必須共享同一套視覺規則：

- header 與 page intro 保持乾淨，不堆疊多層說明框
- 卡片使用一致半徑、陰影、內距
- 主要數值、次要標籤、時間戳、診斷細節有明確字級層次
- 避免再次回到 dense checkbox wall 或一格一格表單方塊感

`/trends` 具體要求：

- 主圖為核心，不可淪為頁面底部附屬元件
- subsystem selector 以精簡列表/row 為主，不做大型表單牆
- 多條曲線可疊圖，但顏色需受控並按 subsystem 有一致語意
- latest-value chips 與圖例需可快速關聯，不迫使操作者反覆對照長文字

`/readback viewer` 具體要求：

- 以保存值與最後刷新狀態為主
- proof/debug 區塊以 details、secondary panel、或明確 debug mode 呈現
- 不再讓單次 job envelope 成為整頁主結構

### 5b. `packet-lab` 必須直接說明「封包哪裡錯」

目前 `packet-lab` 後端已經有：

- `expectedFailureReason`
- `packetSummary.highlight`
- secure header / inner command / auth preview
- observed reject telemetry / reject event evidence

但前端目前仍過度停留在：

- 按一個 case
- 看到失敗/拒絕
- 看到 reason number

這不足以支撐論文 demo，因為操作者與口委無法在同一頁直觀看見：

- 這包原本想模擬哪種錯誤
- 哪個欄位被故意改壞
- 正確值應該是多少
- 實際送出去的是多少
- 最後飛行端以哪一類 reject 拒絕它

這次 `packet-lab` UI 要求固定補上：

- `Packet Fault Callout`
  - 用明確文字與 highlight 呈現這次故意做錯的欄位
- `Expected vs Actual`
  - 至少對 `sequence / session / MAC / source` 這類 case 關鍵欄位顯示
    應有值與實際值
- `Reject Reason Decode`
  - 將 `*_LAST_REJECT_REASON` 的數字代碼轉成對應 enum 名稱
- `Observed Flight Rejection`
  - 在同一區塊顯示 reject 來源是 `SESSION / SEQUENCE / SECURE_COMMAND`
    哪一條路徑
  - 並與地面端推導出的 injected fault model 明確分開

這次不要求完整 raw packet bit-level visualizer，但要求：

- 不切離 `packet-lab` 頁面，操作者就能指出「這包哪裡錯」
- 不需要再切到 raw events 才能理解 reject reason

替代方案：

- 只保留現在的結果卡與 raw evidence：
  - 不足以支撐 thesis/demo 的直觀性。
- 只把 hex dump 展開更多：
  - 仍然不夠直觀，因為口委看不出欄位語意。

### 6. trend 預設只 expose curated continuous operator truth

trend 頁第一版不應直接對全部 runtime channel 開放，否則會把 diagnostics-only
residual chatter 混進 operator 視圖。

第一版預設 channel set 固定為：

- `EPS`
  - `EPS_VBAT`
  - `EPS_IBAT`
  - `EPS_SOC`
  - `EPS_TEMP_BAT`
- `ADCS`
  - `ADCS_Q0`
  - `ADCS_Q1`
  - `ADCS_Q2`
  - `ADCS_Q3`
  - `ADCS_OMEGA_X`
  - `ADCS_OMEGA_Y`
  - `ADCS_OMEGA_Z`
- `Health`
  - `SYS_CPU_USAGE`
  - `SYS_MEM_RSS_MB`

這讓 UI 收斂與 flight-side cleanup 可以解耦：

- 先把 operator 真值畫面做好
- residual live chatter 再逐批清

### 7. residual chatter 先做 inventory + UI default boundary，不在同一輪承諾全停

目前這輪明確記錄的 residual runtime chatter inventory 如下：

- formal reviewable but not default operator view:
  - `GROUND_LINK_UP/DOWN`
  - `GROUND_LINK_TX_BYTES`
  - `GROUND_LINK_HEALTH_S_BAND_*`
  - `QueueOverflow`
  - `CSP_OWNER_TIMEOUT`
  - `CSP_OWNER_TOTAL_TIMEOUTS`
  - S-band `CommEgressMux` counters
- supplemental / diagnostics-only:
  - `SystemResources.*`
  - queue depth / overflow internals not already promoted as formal proof
  - `CspRuntimeOwner` detailed counters
  - `UartDriver` counters
  - `COMM_PASS_*`
  - `COMM_RT_*`
  - `RadioController` raw cached status
- explicitly not promoted into first trend tranche even though related data can
  exist in runtime:
  - `GPS` detailed fields
  - `GPS_SAT_COUNT`

這次先做兩件事：

- 明確 inventory 它們目前是否仍在持續下傳
- 在 Mission Console UI 預設中不把它們升成 operator chart/dashboard truth

本 tranche 的 UI default boundary 固定為：

- `/trends` 第一版只吃 `EPS`、`ADCS`、`Health`
- `/dashboard` 只吃 posture-first curated snapshot truth
- `/readback` 只呈現保存後的 structured readback，不把上述 residual chatter
  重新包裝成主視圖

不在這次 UI tranche 直接承諾把它們全部停乾淨，避免 scope 爆開。

### 8. governed sequence authoring 使用官方 `.seq -> fprime-seqgen -> .bin` 鏈，不自創格式

目前 repo 已可確認的 sequence 正式路徑是：

- 人可讀 source：官方 `.seq`
- 官方編譯器：`fprime-seqgen`
- governed upload：`.sequence-staging/<leaf>.bin`
- governed execution owner：`SequenceAdmissionController`

這次若要補「建立 sequence 檔案的頁面」，就不應自創另一套 JSON-only 或
web-only sequence 格式；頁面應該只是幫 operator 更容易產生與管理官方 `.seq`
與對應 `.bin`。

第一版固定設計為：

- 新增獨立 `/sequences` 頁，而不是把完整 builder 塞回 `/ops`
- 頁面包含：
  - 結構化 step editor
  - `.seq` 原文預覽
  - compile diagnostics
  - upload / validate / run / prepare-manual workflow
- 結構化 editor 第一版只主推相對時間 `R...` step
- 原文預覽保留進階 fallback，讓 operator 仍可直接理解與必要時調整官方文字格式
- drafts 與 compile artifacts 固定保存在
  `MISSION_CONSOLE_ROOT/sequence-drafts/`

原因：

- sequence authoring 是「建立檔案 artifact」流程，不是單次 command dispatch
- 需要比 `/ops` 更大的版面來放 step list、source preview、compile diagnostics
- 和 `/readback` 一樣，這是一個獨立工作面，不適合塞進既有 action form 牆

### 9. sequence builder 的 compile 與 admission 必須明確分開

這個 repo 對 sequence 並不只有「語法合法就能跑」。

在 `SequenceAdmissionController` 上，sequence 還會再經過：

- `.sequence-staging/<leaf>` 路徑治理
- binary / CRC / time base 檢查
- record 反序列化檢查
- 每條 inner opcode 的 authority 檢查
- nested sequencing / admin opcode 禁止

因此這次 sequence 頁面的流程語意要明確拆成兩段：

1. `Compile`
   - 只代表官方 `.seq` 語法與 dictionary 解析成功
   - 產出本地 `.bin`
2. `Governed admission / execution`
   - 代表上傳到 `.sequence-staging/<leaf>.bin`
   - 再透過 `SEQ_VALIDATE / SEQ_RUN / SEQ_PREPARE_MANUAL`
   - 最終由 `SequenceAdmissionController` 決定是否可接受

UI 不可把 compile 成功誤導成 sequence 已可在 flight path 上執行。

### 10. sequence builder 後端應先抽成 repo-owned helper，不重用 probe 內零散 subprocess 片段

目前 repo 內已存在多個 proof/probe 片段各自用 subprocess 呼叫
`fprime-seqgen` 建立測試用 sequence binary，但沒有一個正式 repo-owned 的
Mission Console backend helper。

這次應收斂成：

- Mission Console backend 自己擁有 sequence authoring helper
- helper 直接呼叫官方 `fprime-seqgen`
- dictionary 來源固定跟 active context surface 的 `dictionaryPath` 對齊
- source / output / diagnostics 由 Mission Console workspace 管理

不採用：

- 從 probe script 直接複製一段 subprocess 邏輯進 route handler
- 在前端重寫 `.seq` parser / compiler
- 繞過 dictionary，直接假設 command schema 永遠與某份 sample 相同

### 11. sequence builder 第一版只承諾官方基礎語法與 governed workflow，不承諾 planner

這一頁的定位是：

- sequence source builder
- sequence compile surface
- governed upload / validate / run convenience layer

這一輪不承諾：

- pass planner
- calendar / pass-timeline 編排
- onboard mission scheduler
- sequence library database
- 任意 stock `SeqDispatcher` / `CmdSequencer` 低階控制外露

也就是：

- 頁面可以讓 operator 方便建立並執行官方 sequence
- 但不把 current repo sequencing 說成完整 mission planning system

## Risks / Trade-offs

- [Trend history cache 長太快] → 用 bounded window 與 curated channel set 控制，
  第一版不允許 dictionary 全量勾選。
- [Dashboard 重排時誤刪 operator 需要的欄位] → 先做欄位分級：
  primary posture、subsystem snapshot、secondary autonomy breadcrumb。
- [把 diagnostics-only channel 不小心當成 operator truth] → trend selector 與
  dashboard source 固定走 curated mapping，不直接吃全量 latest snapshot。
- [Readback viewer 與 debug 面重新耦合] → 明確限制 operator 第一層只看保存值、
  刷新狀態、來源指令；proof 細節留到次層。
- [Packet-lab 仍只像 reject logger] → 明確要求 fault callout、expected/actual、
  與 reject reason decode 都留在同一頁主結果面。
- [Sequence compile 成功被誤解成已通過 governed admission] → UI 與 backend
  result 必須明確拆分 compile、upload、validate、run 四段狀態，不用單一
  「success」吞掉整條鏈。
- [Sequence builder 偷偷變成 planner / raw sequencer front-end] → route、文案與
  action 邊界固定只承認官方 `.seq` authoring 與 governed `SEQ_*` workflow，
  不外露 raw `SeqDispatcher` / `CmdSequencer` 控制。
- [視覺收斂退回工程預設樣式] → 在 artifact 層明確要求 spacing-first、
  shadow-over-border、受控色系與圖表主畫面優先。
- [UI tranche 與後續 flight-side chatter cleanup 不同步] → 設計上先把 UI default
  boundary 與 flight-side cleanup 分開；前者先完成，後者再逐批 close。
- [直接依賴 stock GDS chart addon 造成耦合] → 改採 repo-owned vendor 與自己的
  lightweight wrapper。
- [一次動太多頁面導致 scope 失控] → 這一輪承諾 `dashboard + trends +
  ops/readback 分工收斂 + packet-lab demo cleanup + governed sequence authoring`，
  `/surfaces` 後續再逐頁收斂。
