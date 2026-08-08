# 03 — 記憶體、資源與儲存設計

## 1. 文件目的

本文件重新以 **Linux / Raspberry Pi 3B+** 為第一版目標平台，定義：

- OBC 行程資源預算
- simulators 行程資源預算
- CSP / F' buffer 與 thread 規劃
- 持久化儲存、log、staging file、A/B update slot 規劃
- macOS 與 Raspberry Pi 的差異與限制

> 本版已完整移除 STM32、raw flash address、SRAM bank、linker-script 類設計。

## 2. 平台假設

### 2.1 開發平台：macOS

| 項目 | 假設 |
|------|------|
| CPU | Apple Silicon / Intel 均可 |
| 記憶體 | 8 GB 以上常見開發機 |
| 儲存 | 本機磁碟或 SSD |
| 角色 | 純軟體開發、GDS、模擬器、整合測試 |

### 2.2 目標平台：Raspberry Pi 3B+

| 項目 | 假設 |
|------|------|
| CPU | Broadcom BCM2837B0, quad-core ARM |
| RAM | 1 GB |
| 儲存 | microSD card |
| OS | Linux |
| 角色 | OBC 主程式目標運行平台 |

## 3. 資源設計原則

1. 第一版以 **bounded dynamic allocation** 為原則，而非裸機完全禁用動態配置。
2. 必須限制長時間運行下的記憶體成長與 log 膨脹。
3. 資源預算以 **RSS、檔案空間、buffer 數量、thread 數量** 表述。
4. 大檔更新流程使用 staging file，不把映像塞進 command buffer。
5. RPi 目標模式與 macOS 開發模式共享主要資料結構與 buffer 策略。

## 4. 行程與記憶體預算

### 4.1 OBC 主程式預算

| 類別 | 目標值 | 上限 | 說明 |
|------|--------|------|------|
| 常態 RSS | 80 MB | 160 MB | 正常運行與基本遙測 |
| 峰值 RSS | 140 MB | 256 MB | 含 GDS 互動、檔案處理、更新檢查 |
| 執行緒數 | 8-16 | 24 | 含 F' active components / drivers |
| 開啟檔案數 | < 64 | 128 | logs、socket、staging、dictionary |

### 4.2 模擬器預算

| 行程 | 常態 RSS 目標 | 上限 | 備註 |
|------|---------------|------|------|
| EPS simulator | 20 MB | 64 MB | 物理模型 + CSP server |
| ADCS simulator | 32 MB | 96 MB | 動力學、感測器與控制迴路 |
| echo / test node | 10 MB | 32 MB | 測試節點 |
| ZMQ proxy | 16 MB | 64 MB | 視連線數與封包量而定 |

### 4.3 測試模式整體預算

在 macOS 全部節點同機執行時，建議總 RSS 預算控制在 **500 MB 以內**，以利長時間整合測試穩定運行。

## 5. F' 與 CSP 資源規劃

### 5.1 F' buffer manager 預算

第一版採保守但非極小化的 Linux 配置：

| Buffer Pool | 內容 | 目標總量 |
|-------------|------|----------|
| CommsBufferManager | 指令、遙測、事件封包 | 24-40 MB |
| Dp / file-related buffers | 檔案與 data product 路徑 | 16-32 MB |
| GeneralBufferManager | Bridge、控制與中型資料交換 | 8-16 MB |

> 最終數值依實際 F' deployment 與檔案下傳需求微調，但整體不建議超過 **96 MB**。

### 5.2 CSP 資源規劃

建議初始配置：

```c
#define CSP_BUFFER_SIZE     256
#define CSP_BUFFER_COUNT    128
#define CSP_CONN_MAX        16
#define CSP_CONN_QUEUE_LEN  16
#define CSP_FIFO_INPUT      16
#define CSP_RTABLE_SIZE     16
```

### 5.3 CSP 設計理由

- Linux / ZMQ 環境下不需要沿用極端小型 buffer 假設
- `256 x 128` 可兼顧小型控制封包與中型狀態回應
- 若未來導入高頻資料或更多節點，再以量測結果調大

## 6. 執行緒與排程預算

### 6.1 OBC 執行緒分類

| 類別 | 數量估計 | 說明 |
|------|----------|------|
| F' Active Components | 4-8 | RateGroup、command / event / file handling 等 |
| 通訊與 driver threads | 1-4 | TCP / UART / router / monitor |
| 背景維護 | 1-2 | update / health / housekeeping |
| 總數 | 8-16 | 正常範圍 |

### 6.2 Stack 配置原則

Linux 下 stack 以 thread attribute 或系統預設值管理，第一版建議：

| 執行緒類型 | 建議 stack |
|-----------|-----------|
| 一般 active component | 512 KB - 1 MB |
| router / update / file-intensive thread | 1 MB - 2 MB |
| 模型計算 thread | 1 MB |

> 第一版不再維護裸機式每個 thread 的固定 KB 地址預算表，而以 Linux thread 配置與實測 watermark 為準。

## 7. 儲存空間規劃

### 7.1 儲存分區角色

在 Raspberry Pi 3B+ 上，第一版使用「filesystem / partition role」描述，而非 raw flash 位址：

| 區域 | 用途 | 建議預算 |
|------|------|---------|
| Boot / OS base | Linux 啟動與基礎系統 | 依 OS 需求 |
| OBC App Slot A | 穩定版本映像 / 套件 / 啟動設定 | 512 MB - 1 GB |
| OBC App Slot B | 待切換或備援版本 | 512 MB - 1 GB |
| Staging Area | file uplink 後待驗證映像 | 256 MB - 512 MB |
| Persistent Data | 參數、狀態、少量 metadata | 64 MB |
| Logs / Evidence | OBC log、測試證據、GDS 摘要 | 256 MB - 1 GB |

### 7.2 Boot / Update metadata

建議 metadata 使用檔案或輕量資料庫保存，而非固定 flash struct：

- `active_slot`
- `pending_slot`
- `confirmed`
- `last_known_good`
- `expected_digest`
- `last_update_result`

### 7.3 Staging file 規則

- 韌體映像先上傳到 staging area
- 驗證成功後才允許進行 slot activation
- staging file 檔名需含版本或 digest 資訊，避免覆寫混淆

## 8. Log 與證據檔案策略

### 8.1 必要產物

| 類別 | 說明 |
|------|------|
| OBC runtime logs | 啟動、事件、錯誤摘要 |
| simulator logs | EPS / ADCS / echo node 摘要 |
| GDS interaction logs | 指令與驗證用摘要 |
| test records | 人工檢查紀錄，格式見 [10_test_record_guide.md](./10_test_record_guide.md) |

### 8.2 管理原則

- 不要求保存所有原始 stdout 全量輸出
- 驗證紀錄只需保留 **摘要證據**
- log 需設 rotation 或定期清理策略，避免長期填滿 SD card

## 9. macOS 與 Raspberry Pi 差異

| 項目 | macOS 開發模式 | Raspberry Pi 目標模式 |
|------|----------------|----------------------|
| 記憶體餘裕 | 高 | 中等 |
| 儲存媒介 | SSD / 本機磁碟 | microSD |
| 主要用途 | 開發、整合、GDS | 實際運行、目標機驗證 |
| 外部鏈路 | TCP mock 為主 | TCP mock 或 UART |
| 內部 CSP | ZMQ | ZMQ |

### 9.1 不應出現的錯誤假設

- 不應把 Raspberry Pi 視為 STM32 類 MCU
- 不應在本版規格中描述 raw flash sector address
- 不應把 Linux process memory 誤寫成固定 SRAM bank 配置

## 10. 監控與告警

### 10.1 建議監控項目

| 項目 | 指標 | 建議告警 |
|------|------|---------|
| OBC RSS | `SYS_MEM_RSS_MB` | > 160 MB |
| CPU 使用率 | `SYS_CPU_USAGE` | > 85% 持續 60 秒 |
| CSP free buffers | `CSP_FREE_BUFFERS` | < 16 |
| staging 剩餘空間 | storage monitor | < 2 倍映像大小 |
| log 區容量 | storage monitor | > 80% |

### 10.2 行動原則

- warning：記錄事件並繼續運作
- alarm：限制非必要功能或切換低負載模式
- fatal：僅在確定會造成資料損毀或無法恢復時觸發

## 11. `Blocked-HW` 與替代驗證

第一版允許以下項目標記為 `Blocked-HW`：

- 實體 UART 線路收發
- 真實 radio enable / telemetry
- 真實 GPIO / I2C 驗證

每項 `Blocked-HW` 至少要提供其中一種替代證據：

1. TCP mock 或 PTY 模擬結果
2. unit / component test 結果
3. GDS / script 驗證摘要
4. code path 與 error handling 的靜態檢查說明

## 12. 驗收準則

| 編號 | 驗收項目 | 通過條件 |
|------|---------|---------|
| 1 | 平台描述一致 | 全文只描述 Linux / RPi 資源模型 |
| 2 | 移除 STM32 痕跡 | 不含 raw flash / SRAM bank / DTCM |
| 3 | Update 空間定義一致 | 有 slot A/B、staging、persistent、logs |
| 4 | 記憶體預算可操作 | 以 RSS / buffer / thread / storage 表達 |
| 5 | 測試限制可追蹤 | `Blocked-HW` 條件與替代證據清楚 |
