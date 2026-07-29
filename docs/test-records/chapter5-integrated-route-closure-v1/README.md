# 測試紀錄：chapter5-integrated-route-closure-v1

## Scope

- Route 2/3 current formal rerun root：`2026-06-28`
- Route 1 thesis-backed functional observation：`2026-07-20` (`functional PASS`)
- Route 1 2026-07-12 historical proof：not current target authority
- Route 1 target requalification：pending
- originating historical closure dates：`2026-06-25`、`2026-06-28`
- historical change archive：
  `openspec/changes/archive/2026-06-24-chapter5-integrated-route-closure-v1/`
- historical follow-on dependency：
  `openspec/changes/archive/2026-06-24-uhf-primary-nonquiet-autofailover-v1/`

這份 README 是 Chapter 5 三條整合驗證路線的 durable entrypoint。current truth
以 fresh rerun 與 repo-backed raw artifacts 為準；舊 `/tmp` /`/private/tmp`
retained evidence 只保留 historical review value。

本文件已對照 Route 2/3 的 `2026-06-28-formal-rerun` 及 Route 1 的
`2026-07-20-route1-target-abc-rerun` raw tree。若要引用資料寫論文，先依下列
Route 1 status 判別 target claim 的範圍，再進入對應 raw tree。

Current-note:

- Route 1 的 2026-06-28 non-sequence raw tree 是 retained historical evidence；
  它不能取代 current public-surface 的 sequence rerun
- Route 1 的 2026-07-20 target observation 保留完整、未壓縮 raw evidence；
  它保留 target sequence、downlink 與 SoC fallback 的論文觀測值及完整 OBC
  journal，但 C 曾重啟 shared OBC service，且 contemporaneous revision
  provenance 不完整，因此不是 current governed A/B/C closure。本次沒有重跑
  hosted，也不得將它表述為新的 hosted claim
- target real-camera source-image validity 另由
  [payload-target-capture-sanity-v1](../payload-target-capture-sanity-v1/README.md)
  承接；不要把兩者混成同一個 claim

## Canonical Artifact Root

- Route 1 thesis-backed target functional observation（functional `PASS`）：
  [2026-07-20-route1-target-abc-rerun](ARTIFACTS.json)
- Route 1 decoded-JSON canonicalization and retained-artifact hashes：
  [dedup-manifest.json](ARTIFACTS.json)
- Route 2/3 current formal rerun root：
  [2026-06-28-formal-rerun](ARTIFACTS.json)
- per-route surfaces：
  - [Route 1 target functional observation](ARTIFACTS.json)
  - [route2/hosted](ARTIFACTS.json)
  - [route2/target](ARTIFACTS.json)
  - [route3/hosted](ARTIFACTS.json)
  - [route3/target](ARTIFACTS.json)

每個 surface 目錄都包含：

- current formal wrapper root copy
- discovered nested stage roots copy
- `manifest.json`
- retry / note / command metadata

`2026-06-28-formal-rerun` 是這輪 canonical campaign root 名稱，不等於每個
surface 都在 `2026-06-28` 同時完成。各 surface 的實際 rerun 時間，以各自
`manifest.json` 內的 `timestamp` 為準；例如 Route 1 hosted/target 是後續在
`2026-06-29` 刷新後回填到同一棵 canonical artifact tree。

## How To Read This Record

建議固定用下面順序查資料：

1. 先看對應 surface 的 `manifest.json`
   - 確認 rerun command、timestamp、retry 次數、正式 verdict
   - 確認 `wrapper-root/` 與 `external-roots/` 分別對應哪些原始 tmp 路徑
2. 再看 `wrapper-root/.../summary.log`
   - 這是該 route surface 的聚合結果
   - 適合先快速理解 staged flow 是否全部過關
3. 若要引用單一 stage 或單一觀測面，再進 `external-roots/`
   - target staged route 重要 stage summary 通常在這裡
   - downlink 產物、payload 檔、HK decode、failover proof 也都從這裡進
4. 若要追失敗或詳細時序，再看 wrapper root 內的原始 log
   - 例如 `events.log`、`raw-command.log`、`gateway.log`、`obc.log`
   - Route 3 target 另外可看 `diagnostics/checkpoints.jsonl`

`wrapper-root` 的角色是「這次 formal rerun 的聚合執行現場」；
`external-roots` 的角色是「wrapper 在執行時引用到的 nested stage 證據或產品檔案」。

## Artifact Provenance Legend

為避免把「target 上原本存在、之後被抓回來的檔案」誤當成「地面已實際收到的證據」，
這次 formal rerun 的 canonical artifact tree 可用下面規則判讀來源：

- `wrapper-root/...`
  - rerun wrapper 本身在 macOS 上執行時產生的聚合 log / summary / control logs
  - 這是 orchestrator / probe 視角，不是 target onboard storage 本體
- `.../case/source-artifacts/...`
  - target 上已存在的 onboard artifact，之後由 SSH 或 target-side helper 收回
  - 這些代表「衛星端曾生成並存放」
- `.../case/source-decode/...`
  - 以上 `source-artifacts` 或 target-side `.fdp` 在 macOS 上再 decode / extract 的衍生物
  - 它們對應 target-resident source，不代表地面鏈路已收到
- `.../case/sband-ground/...`、`.../case/uhf-ground/...`
  - macOS ground stack / GDS / gateway 實際收到的鏈路證據
  - 這些才屬於 ground-received 路徑
- `.../case/received-decode/...`
  - 對 `sband-ground` 或 `uhf-ground` 實際收到的 `.fdp` 再 decode / extract 的衍生物
  - 這些代表「地面實際收到了什麼」

寫論文時若要主張「target 有生成資料」，優先引用 `source-artifacts` /
`source-decode`；若要主張「地面成功收到了資料」，優先引用 `sband-ground` /
`uhf-ground` / `received-decode`。

## Per-Route File Guide

### Route 1

#### Thesis-Backed Target Functional Observation (2026-07-20)

先看：

- [manifest](ARTIFACTS.json)
- [dedup manifest](ARTIFACTS.json)
- [reading guide](ARTIFACTS.json)
- [target wrapper summary](ARTIFACTS.json)
- [complete OBC journal](ARTIFACTS.json)

這次直接在剛完成的部署狀態執行 target，並在 C 前後呼叫 A/B，沒有 rebuild、
package 或 install。S-band secure auth、mode entry、sequence upload/validate/run、
AUTO/DETERMINISTIC metadata、deterministic preview ground receipt，以及 SoC
fallback 的 functional observation 都為 `PASS`；完整 OBC journal 已一併封存。
但 C 安裝／移除 `56-obc-groundlink-timeouts.conf` 時重啟 shared OBC service，
且 contemporaneous revision provenance 不完整，因此此 campaign 不符合 current
A/B/C authority。`route1-sequence-verification-v1` 記載的 2026-07-12 proof
同樣缺少可獨立檢查的 retained remote workspace/build/install provenance，
因此只保留歷史功能證據地位。此 campaign 沒有 fresh hosted rerun，不能擴張為
hosted current closure。只有
SHA-256 完全相同的 decoded JSON 衍生副本透過 dedup manifest canonicalize；
transport bytes、source/received FDP、provenance 與 verdict oracle 均保留。

#### Historical Non-sequence Hosted (2026-06-28 Tree)

建議先看：

- [manifest.json](ARTIFACTS.json)
- [summary.log](ARTIFACTS.json)
- [payload-raw-preview-dual-artifact-summary.json](ARTIFACTS.json)

若要看 payload 原始與解碼結果：

- [PIC10.bin](ARTIFACTS.json)
- [PIC10.jpg](ARTIFACTS.json)
- hosted `.fdp` files under
  [external-roots](ARTIFACTS.json)

來源說明：

- hosted Route 1 沒有 target onboard filesystem / SSH 這層 distinction
- 這裡的 payload 檔案是 hosted runtime local artifact
- 這次 only target 真機 rerun 改成 `exposure_usec=30000` / `gain_x100=400`
- hosted 仍是 stub camera slice，不可拿 hosted 檔頭去主張 OBC 真機相機參數
- downlink 相關 log 仍要搭配 wrapper root 內的 GDS/gateway logs 一起讀

#### Historical Non-sequence Target (2026-06-28 Tree)

建議先看：

- [manifest.json](ARTIFACTS.json)
- [summary.log](ARTIFACTS.json)
- [route1-soc-fallback-summary.json](ARTIFACTS.json)
- [campaign-summary.json](ARTIFACTS.json)
- [payload-raw-preview-dual-artifact-target-summary.json](ARTIFACTS.json)

這裡容易誤讀，先明確說明：

- Route 1 current formal target evidence 只有一個 surface
  - [manifest.json](ARTIFACTS.json) 的正式 verdict 是唯一 current target 結論
- `wrapper-root/chapter5-route1-refresh-target-retry`
  - 表示這個 formal surface 在允許政策內做過一次 fresh retry，最後成功的是第二次完整 `A -> B -> C`
- `external-roots/chapter5-route1-target-fallback.epaefV`
  - 是同一次 formal target surface 下面的 SoC fallback stage 證據
- `external-roots/payload-raw-preview-dual-artifact-v1-target.xw9FPi`
  - 是同一次 formal target surface 下面的 payload capture/downlink stage 證據

也就是說，這裡不是「兩次 current target rerun 並列保存」，而是「一個 current
target rerun surface，聚合兩段 staged evidence」。

若要引用 source/downlinked `.fdp` 對照：

- [source `.fdp` on target side](ARTIFACTS.json)
- [downlinked `.fdp` received on ground](ARTIFACTS.json)

Route 1 target 重要來源區分：

- `source-artifacts/.../PIC20.jpg`
  - target onboard 檔案，之後被抓回
- `source-decode/.../Dp_268673025_1782664067_00412927.jpg`
  - 對 target-side source `.fdp` 解出的 JPEG
- `sband-ground/.../fprime-downlink/...00412927.fdp`
  - 地面實際收到的 `.fdp`
- `received-decode/.../_home_youjun_obc-deploy_runtime_comm-csp-lab-obc_data-products_Dp_268673025_1782664067_00412927.jpg`
  - 對 ground-received `.fdp` 解出的 JPEG

這次 formal rerun 中，上面三個 JPEG / FDP family 的 `sha256` 一致，可同時支持：

- target 確實生成 preview JPEG
- ground 確實收到了相同內容的 preview JPEG data product
- 這次 target rerun 的真機 payload 參數已更新為 `exposure_usec=30000` / `gain_x100=400`
- target surface 發生過一次 fresh retry；原因已記在 Route 1 target manifest 的 `retryReasons`
### Route 2

#### Hosted

建議先看：

- [manifest.json](ARTIFACTS.json)
- [summary.log](ARTIFACTS.json)
- [route2-mode-ttc-entry-hosted.log](ARTIFACTS.json)

若要看 beacon/HK 證據：

- [beacon-capture.bin](ARTIFACTS.json)
- [beacon-decoded.json](ARTIFACTS.json)
- [decoded HK JSON](ARTIFACTS.json)

#### Target

建議先看：

- [manifest.json](ARTIFACTS.json)
- [summary.log](ARTIFACTS.json)
- [autonomous-uhf-failover-summary.json](ARTIFACTS.json)

這條 route 的 target closure 是兩段聚合：

- TTC entry + ADCS `POINTING` 在 wrapper `summary.log`
- autonomous failover + UHF re-auth + HK readback 在
  `autonomous-uhf-failover-summary.json`

來源說明：

- `wrapper-root/route2-target-fixed/summary.log`
  - macOS rerun wrapper 的聚合 verdict，負責把 TTC stage 與 failover stage
    收斂成 current Route 2 target 結論
- `external-roots/autonomous-failover-proof/diagnostics/journal-snapshots/` 與
  `service-snapshots/`
  - target-side / subsystem-side journal 與 service 狀態，屬於 SSH 抓回的
    target evidence，不代表地面已看到相同資訊
- `external-roots/autonomous-failover-proof/sband-ground/` 與 `uhf-ground/`
  - macOS ground stack 實際收到的 packet / readback / `.fdp` 證據
- `external-roots/autonomous-failover-proof/source-snapshots/`
  - target 端 source file 被抓回的快照；這能證明 target 端有生成檔案，
    但若要主張 failover 後地面真的收到 HK `.fdp`，仍應優先引用
    `uhf-ground/gds-files/...`

### Route 3

#### Hosted

建議先看：

- [manifest.json](ARTIFACTS.json)
- [summary.log](ARTIFACTS.json)
- [recovery-executors-v1-probe.log](ARTIFACTS.json)

這裡只證明 hosted partial closure：
watchdog-source `R2`、ADCS `R3`、EPS `R3/R5`，不包含 hosted `R6`。

#### Target

建議先看：

- [manifest.json](ARTIFACTS.json)
- [summary.log](ARTIFACTS.json)
- [status.log](ARTIFACTS.json)
- [post-r6-secure-auth/status.log](ARTIFACTS.json)

若要看 watchdog 後 readback 與 readback command flow：

- [events.log](ARTIFACTS.json)
- [raw-command.log](ARTIFACTS.json)
- [checkpoints.jsonl](ARTIFACTS.json)

來源說明：

- `wrapper-root/route3-target/summary.log`、`status.log`、`service-status.log`
  - macOS rerun wrapper 的聚合結果與 stage-level status
- `wrapper-root/route3-target/diagnostics/journal-snapshots/`、
  `service-snapshots/`、`checkpoints.jsonl`
  - target-side / subsystem-side SSH 抓回的 recovery breadcrumbs
- `wrapper-root/route3-target/sband-ground/`
  - pre-reboot current readback / command-path 的地面實收證據
- `wrapper-root/route3-target/post-r6-secure-auth/sband-ground/`
  - watchdog reboot 後重新 secure-auth 與 `GET_RESET_CAUSE` /
    `GET_HW_WATCHDOG_STATUS` / `GET_WATCHDOG_STATUS` /
    `GET_PERSISTENT_FAULT_HISTORY` 的地面實收證據
- 若要主張 reboot 後地面真的看到了 readback，不應只引用
  `journal-snapshots`；要搭配 `post-r6-secure-auth/sband-ground/events.log`、
  `raw-command.log` 或對應 `cli-logs/events/*`

## Current Formal Rerun Summary

| Route | Hosted | Target | Current formal status |
| --- | --- | --- | --- |
| Route 1: payload + SoC fallback + `.fdp` downlink | not rerun in this campaign | 2026-07-20 functional observation `PASS` | 2026-07-12 and 2026-07-20 are historical/non-authoritative; target requalification pending |
| Route 2: `SAFE/HELL/SAFE/IDLE/TTC` + ADCS + link/HK | `PASS` | `PASS` | closed on current maintained path |
| Route 3: recovery + reboot | hosted partial `PASS` by design | `PASS` | closed on current maintained path |

## Route 1

### Hosted

- 本次未重跑 hosted surface；本 target-only record 不更新 hosted verdict，也不以
  target journal 替代 hosted evidence。

### Target

- thesis-backed sequence artifacts：
  [2026-07-20 target functional observation](ARTIFACTS.json)
- 2026-07-20 functional verdict：`PASS`（not governed A/B/C authority）
  - current S-band auth and node-5 V3 downlink profile (`240` bytes) passed
  - sequence run generated AUTO index 48 and DETERMINISTIC index 49 evidence
  - deterministic preview `.fdp` was received and decoded on the macOS ground
  - 59% `PAYLOAD -> IDLE` and 39% `IDLE -> SAFE` SoC fallbacks passed
  - complete OBC journal and the bounded `SEQ_VALIDATE` target-journal window
    are retained under `prepare-and-capture/scenario/diagnostics/journal-snapshots/`

## Route 2

### Hosted

- current formal artifacts：
  [route2/hosted](ARTIFACTS.json)
- current hosted truth remains split:
  - `route2_mode_ttc_entry`：hosted auth-neutral runtime control
  - `route2_link_recovery_and_hk`：hosted dual-link/HK reuse, not hosted secure-auth evidence
- current verdict：
  - `route2_mode_ttc_entry: PASS`
  - `route2_link_recovery_and_hk: PASS`
  - `chapter5-route2-hosted: PASS`
- retry note：
  - current hosted formal rerun needed bounded probe hygiene only
  - no product semantics were changed
  - the final rerun uses:
    - hosted node-`6` stand-in startup
    - event-first local TTC oracle
    - broad TTC pass-window programming
    - short independent stage roots for HK to avoid AF_UNIX path overflow and
      stage-root cross-talk

### TTC / ADCS Hook Design

current implementation in repo:

- `TtcPassManager` 在 TTC entry 成功後 best-effort 送一次 internal ADCS mode
  switch to `POINTING`
- 這個 hook 失敗不阻擋 TTC mode entry
- 不做 TTC exit restore
- target tracking law / quaternion control 不在這個 slice 內

current code surface:

- [TtcPassManager.cpp]($REPO_ROOT/OBC/Components/TtcPassManager/TtcPassManager.cpp)
- [AdcsBridge.cpp]($REPO_ROOT/OBC/Components/AdcsBridge/AdcsBridge.cpp)

### Target

- current formal artifacts：
  [route2/target](ARTIFACTS.json)
- current verdict：
  - `route2_mode_ttc_entry: PASS`
  - `route2_link_recovery_and_hk: PASS`
  - `chapter5-route2-target: PASS`
- current closure facts：
  - `SAFE -> HELL -> SAFE -> IDLE -> TTC`
  - TTC auto-entry and ADCS `POINTING`
  - detector-triggered autonomous failover to `UHF primary`
  - `UHF` re-auth
  - bounded HK/downlink closure on the current maintained path
- retry note：
  - TTC stage needed one fresh rerun because current ground channel snapshot did
    not stably materialize `TTC_POLICY_CURRENT_GPS_UNIX_SEC`
  - current maintained wrapper now tolerates that oracle miss with host-time
    broad-window fallback while still requiring actual TTC-entry / ADCS-pointing
    proof

## Route 3

### Hosted

- current hosted scope remains intentionally partial：
  - watchdog-source `R2`
  - ADCS `R3`
  - EPS `R3/R5`
  - no hosted hardware-watchdog `R6` claim
- current formal artifacts：
  [route3/hosted](ARTIFACTS.json)
- current verdict：
  - `adcs_r3_reset: PASS`
  - `eps_timeout_fdir: PASS`
  - `chapter5-route3-hosted-pre-reboot: PASS`

### Target

- current formal artifacts：
  [route3/target](ARTIFACTS.json)
- current verdict：
  - `route3_adcs_r3_first_fault_and_clear: PASS`
  - `route3_eps_r3_safe_fallback: PASS`
  - `route3_watchdog_reboot_and_postcheck: PASS`
  - `chapter5-route3-target: PASS`
- current closure facts：
  - ADCS first-fault `R3` closure retained no-`R2` proof
  - EPS first-fault `R3/R5` closure retained `SAFE_FALLBACK`
  - watchdog `R6` closure retained post-reboot secure-auth readbacks:
    `GET_RESET_CAUSE`, `GET_HW_WATCHDOG_STATUS`,
    `GET_WATCHDOG_STATUS`, `GET_PERSISTENT_FAULT_HISTORY`
- retry note：
  - EPS stage used one fresh stage retry after the first active-recovery event
    proof miss; retry then passed on the same current maintained path

## Adjacent Durable Records

- non-quiet `UHF primary` runtime benchmark：
  [uhf-primary-nonquiet-runtime-v1](../uhf-primary-nonquiet-runtime-v1/README.md)
- autonomous failover dedicated record：
  [target-autonomous-uhf-failover-v1](../target-autonomous-uhf-failover-v1/README.md)
- Route 3 pre-reboot dedicated record：
  [target-route3-pre-reboot-recovery-v1](../target-route3-pre-reboot-recovery-v1/README.md)
- Route 3 watchdog dedicated record：
  [target-hardware-watchdog-reset-proof-v1](../target-hardware-watchdog-reset-proof-v1/README.md)
- Route 1 target payload preview/downlink dedicated record：
  [payload-raw-preview-dual-artifact-v1](../payload-raw-preview-dual-artifact-v1/README.md)

## Historical Retained Evidence

- earlier 2026-06-25 / pre-rerun closures remain retained only as historical
  previous closure
- old direct `/tmp` / `/private/tmp` references in adjacent records are kept
  for review value; current Route 2/3 truth is the repo-backed
  `2026-06-28-formal-rerun` tree, while Route 1's thesis-backed target
  functional observation is retained at `2026-07-20-route1-target-abc-rerun`
- earlier blockers that were already closed by follow-on work remain historical:
  - single-shot non-quiet `UHF primary` readback instability
  - early failover oracle bug around beacon-suppress interpretation
  - stale target watchdog-disable drop-in
  - wrong-host / wrong-COMM-CAN debug contamination

## Verdict

- Route 1 hosted: `PASS`
- Route 1 target: 2026-07-12 historical functional `PASS` and 2026-07-20
  functional observation `PASS`; neither is current authority because retained
  provenance/ownership requirements are incomplete
- Route 2 hosted: `PASS`
- Route 2 target: `PASS`
- Route 3 hosted: partial `PASS` only by design
- Route 3 target: `PASS`

因此 current Chapter 5 formal rerun state 是：

- Route 1：hosted 與 target 的 current sequence closure 均 PASS
- Route 2：closed
- Route 3：hosted remains partial by design；target closed
