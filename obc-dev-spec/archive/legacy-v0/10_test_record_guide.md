# 10 — 測試紀錄指引

## 1. 文件目的

本文件提供人工測試與半自動測試的紀錄模板，供後續人工檢查與專案驗收使用。

> 原則：**記錄步驟、指令、預期與摘要結果即可，不要求貼上全部原始輸出。**

## 2. 何時必須填寫

以下情境必須產出測試紀錄：

1. L3 整合測試
2. L4 系統場景測試
3. 任一 `Blocked-HW` 測試
4. 以 GDS 手動下指令驗證的流程
5. Boot & Update Manager 切換 / 回滾驗證

## 3. 紀錄欄位

### 3.1 基本資訊

- 測試名稱
- 日期時間
- 執行人 / agent
- 測試層級（L1 / L2 / L3 / L4）
- 目標環境（macOS / Raspberry Pi / 模擬）
- 關聯文件

### 3.2 測試步驟欄位

每一步至少記錄：

1. 步驟編號
2. 使用指令或操作
3. 預期結果
4. 實際摘要結果
5. 判定（Pass / Fail / Blocked-HW）

## 4. 建議模板

```markdown
# 測試紀錄：<測試名稱>

- 日期：YYYY-MM-DD HH:MM
- 層級：L3
- 環境：macOS / dev-macos
- 關聯文件：08_testing_verification.md、06_comm_subsystem.md

## 前置條件
- 已啟動 ZMQ proxy
- 已啟動 OBC deployment
- 已啟動相關 simulator

## 測試步驟

| 步驟 | 指令 / 操作 | 預期結果 | 實際摘要結果 | 判定 |
|------|-------------|----------|--------------|------|
| 1 | `CSP_INIT 1` | CSP 初始化成功 | 收到初始化成功事件 | Pass |
| 2 | `CSP_PING 2 1000` | Ping EPS 成功 | 顯示 RTT 約 3 ms | Pass |
| 3 | `EPS_SET_PDU 0 false` | CH0 關閉 | `EPS_PDU_STATUS` bit 改變 | Pass |

## 問題與觀察
- 無

## 結論
- 本測試通過
```

## 5. `Blocked-HW` 模板

```markdown
# 測試紀錄：<測試名稱>

- 判定：Blocked-HW
- 阻塞原因：缺少 Raspberry Pi 3B+ 與開發機之間的 UART 實體連接線

## 原定測試目的
- 驗證 `UartDriver` 在真實線路上的收發行為

## 替代驗證
- 使用 `socat` 建立 PTY pair
- 驗證 driver 的開啟、傳送、錯誤恢復行為

## 替代驗證摘要結果
- PTY 模擬下可成功收發與重連

## 後續解除條件
- 取得實體線材與對應裝置後，改於 Raspberry Pi 目標模式重跑
```

## 6. GDS 測試紀錄建議

若透過 GDS 手動下指令：

- 記錄指令名稱與參數
- 記錄可見 telemetry / event 摘要
- 若使用 CLI 或 test API，也記錄對應命令列或 script 名稱

## 7. 結論與保存原則

- 測試紀錄屬正式驗收產物
- 可保存於 `evidence/records/` 或其他明確目錄
- 若為長期專案，建議每個 phase 建立獨立子目錄
