## Why

目前 Mission Console 的觀測面分類已收斂成四類：

- `持續下傳`
- `狀態變更下傳`
- `GET_* 強制再送一次`
- `診斷 / 檢視`

但 shared `update on change` telemetry-backed `GET_*` 仍缺少正式契約：
若 `GET_*` 只是重寫 OBC 端快取，而值又剛好沒有變，地面就可能完全收不到
新的下傳證據。

## What Changes

- 定義哪些現有欄位應歸入 `持續下傳`、`狀態變更下傳`、`GET_* 強制再送一次`、
  或 `診斷 / 檢視`
- 為 current baseline context 新增一條明確刷新 / 讀回契約：
  `GET_*` 指令必須產生新的可下傳樣本
- 對 shared 主操作狀態 telemetry，優先採用「在同一個既有 channel 上強制重送樣本」，
  而不是預設拆成第二個 surface
- 只有本來就屬於查詢結果 / 歷史回報語意的 surface 才保留 event-based 回應
- 釐清 Mission Console 各頁面的契約，讓 dashboard、讀回頁與工程檢查面不再共用同一個模糊的遙測判定依據

## Capabilities

### New Capabilities

- `mission-console-observability-bootstrap`
  - 規範面向操作者的狀態查詢在明確刷新 / 讀回時，必須產生新的可下傳樣本

### Modified Capabilities

- `mission-console`
  - 收緊 Mission Console 的觀測面需求，讓摘要狀態、明確讀回與工程診斷面依契約分離，而不是繼續依賴目前偶然形成的行為

## Impact

- affected docs:
  - `docs/roadmap/mission-console-observability-recommendations.md`
  - Mission Console runbook / handoff / evidence references as needed
- affected Mission Console code paths:
  - 讀回流程編排
  - 可能新增的操作者可見刷新動作
- affected OBC-facing code paths:
  - `GET_*` handlers that currently only rewrite shared `update on change`
    telemetry
  - component-local helpers for 在既有 channel 上強制送出遙測樣本
- potential future OBC-facing follow-up:
  - 若目前重用詳細 `GET_*` 仍然過重，後續可為 `ADCS`、`EPS`、`COMM` 補較輕量的 status surface
