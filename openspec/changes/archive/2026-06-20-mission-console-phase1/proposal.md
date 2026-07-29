## Why

目前 repo 的受維護地面操作面仍以雙 stock `fprime-gds` 加上外部
`manual_secure_ops` CLI 為主。這條路徑對工程驗證足夠，但對日常操作與論文
demo 都不理想：secure auth、secure command、governed upload、sequence
都需要切回命令列，重要狀態也缺少一個能直接呈現 mode、COMM、session、
recovery、payload 與 readback 的整合畫面。

現在需要一個 repo-owned `Mission Console` Phase 1，把既有 secure/operator
surface 提升成可程式呼叫的 Mission Gateway，並提供一個能支撐 operator
workflow 與 thesis demo 的 dashboard/readback UI，同時保持 current baseline
authority 不變。

## What Changes

- 新增 repo-owned `Mission Console` Phase 1，採 `Flask + server-rendered HTML/JS`
  提供 dashboard、operator actions、detailed readback、surface 狀態頁面。
- 將 `scripts/manual_ops/manual_secure_ops.py` 重構為可匯入的結構化 action layer，
  同時保留原 CLI 行為與 JSON 輸出。
- 新增 Mission Gateway runtime layer，負責 surface registry、job execution、
  `ensure-auth` workflow、listener ownership、snapshot cache、readback parsing、
  action history。
- 新增 gateway-owned `fprime-cli events/channels` listener，避免依賴 current
  manual surface 是否常駐 passive listeners。
- 新增 `packet-lab` demo/diagnostic surface，可故意送 replay、stale-session、
  duplicate-sequence、tampered-sequence、tampered-mac 封包，並顯示關鍵欄位與錯誤點。
- 將 `docs/roadmap/mission-console-phase1-handoff.md` 納入 Phase 1 的正式 companion
  reference，要求實作與後續規劃不可默默偏離其 baseline/authority 結論。

## Capabilities

### New Capabilities
- `mission-console`: Repo-owned Mission Gateway + Mission Console surface for the maintained manual dual-GDS baseline, including structured operator actions, dashboard/readback views, gateway-owned listeners, and bounded negative packet demo tooling.

### Modified Capabilities
- `comm-subsystem`: Extend the maintained operator-surface contract to include the Mission Console as a repo-owned consumer of existing secure/operator authority without introducing a second command plane.
- `interface-contract-index`: Document the Mission Console data boundaries, readback categories, packet-lab diagnostic surface, and the reference status of the mission-console handoff document.

## Impact

- Affected code: `scripts/manual_ops/manual_secure_ops.py`, new `scripts/mission_console/`
  app and gateway modules, related tests, and supporting docs.
- Affected APIs: new local Flask HTTP/HTML surface plus importable structured action
  helpers replacing CLI-only coupling.
- Dependencies: Flask and existing `fprime-cli` / `fprime-gds` tooling already present
  in `fprime-venv`; no React/Vite adoption in Phase 1.
- Systems: hosted manual dual-GDS path first, then target-manual-ground parity using the
  same authority and manifest/status/session contracts.
