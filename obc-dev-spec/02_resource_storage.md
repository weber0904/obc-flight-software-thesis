# 02 — 資源、儲存與持久化基線

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/resource-storage/spec.md`](../openspec/specs/resource-storage/spec.md)。

## 1. 文件角色

本文件定義 Linux / Raspberry Pi 3B+ 導向的資源預算、buffer 配置、儲存空間、持久化 metadata、監控與證據保留規則。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/03_memory_resource_design.md` | 記憶體、buffer、threads、storage、logs、監控與替代驗證 |
| `archive/legacy-v0/07_boot_update_manager.md` | Boot metadata 的儲存角色與 update data path 邊界 |

## 3. 平台假設

| 項目 | 開發平台 | 目標平台 |
|---|---|---|
| 主要系統 | macOS | Raspberry Pi 3B+ / Linux |
| 記憶體餘裕 | 高 | 中等 |
| 儲存媒介 | SSD / 本機磁碟 | microSD |
| 主要用途 | 開發、GDS、simulators、整合測試 | OBC 主程式運行與目標整合 |

規則：

- 第一版資源模型以 Linux process / filesystem 思維描述。
- 不再以 STM32 raw flash、固定 SRAM bank、DTCM 等裸機概念撰寫規格。

## 4. 記憶體與執行緒預算

### 4.1 OBC 主程式

| 類別 | 目標值 | 上限 |
|---|---|---|
| 常態 RSS | 80 MB | 160 MB |
| 峰值 RSS | 140 MB | 256 MB |
| 執行緒數 | 8-16 | 24 |
| 開啟檔案數 | < 64 | 128 |

### 4.2 模擬器與整體測試模式

| 行程 | 常態 RSS 目標 | 上限 |
|---|---|---|
| EPS simulator | 20 MB | 64 MB |
| ADCS simulator | 32 MB | 96 MB |
| echo / test node | 10 MB | 32 MB |
| ZMQ proxy | 16 MB | 64 MB |

同機整合測試時，總 RSS 建議控制在 `500 MB` 以內。

### 4.3 Stack 原則

| 執行緒類型 | 建議 stack |
|---|---|
| 一般 active component | 512 KB - 1 MB |
| router / update / file-intensive thread | 1 MB - 2 MB |
| 模型計算 thread | 1 MB |

## 5. F' 與 CSP buffer 基線

### 5.1 F' buffer manager

| Buffer Pool | 內容 | 目標總量 |
|---|---|---|
| CommsBufferManager | command / telemetry / event 封包 | 24-40 MB |
| Dp / file-related buffers | file handling / data product 路徑 | 16-32 MB |
| GeneralBufferManager | bridge、控制與中型資料交換 | 8-16 MB |

整體不建議超過 `96 MB`。

### 5.2 CSP 初始配置

```c
#define CSP_BUFFER_SIZE     256
#define CSP_BUFFER_COUNT    128
#define CSP_CONN_MAX        16
#define CSP_CONN_QUEUE_LEN  16
#define CSP_FIFO_INPUT      16
#define CSP_RTABLE_SIZE     16
```

## 6. 儲存空間與持久化基線

### 6.1 儲存角色

| 區域 | 用途 | 建議預算 |
|---|---|---|
| Boot / OS base | Linux 啟動與基礎系統 | 依 OS 需求 |
| OBC App Slot A | 穩定版本或待切換版本 | 512 MB - 1 GB |
| OBC App Slot B | 備援或待切換版本 | 512 MB - 1 GB |
| Staging Area | 待驗證映像與大檔 | 256 MB - 512 MB |
| Persistent Data | 參數、狀態、metadata | 64 MB |
| Logs / Evidence | OBC logs、測試證據、摘要紀錄 | 256 MB - 1 GB |

### 6.2 Boot metadata v1

Boot metadata v1 固定採 **persistent data directory 下的檔案式儲存**，不引入資料庫。至少需保存：

- `active_slot`
- `pending_slot`
- `last_known_good_slot`
- `confirmed`
- `expected_digest`
- `last_boot_attempt_time`
- `last_error_code`

### 6.3 安裝版 release layout

Raspberry Pi 安裝版流程需將 immutable release payload 與 mutable runtime data 明確分離。第一版 install root 基線如下：

```text
<install-root>/
├── current -> releases/<release-id>
├── releases/
│   └── <release-id>/
│       ├── bin/
│       ├── dict/
│       ├── launch/
│       ├── manifest.json
│       └── meta/version.json
└── runtime/
    └── integ-rpi/
        ├── persistent-data/
        ├── staging/
        └── logs/
```

規則：

1. `releases/<release-id>/` 只保存 versioned payload，不寫入 mutable state。
2. `current` 需指向目前啟動的 release，讓 operator 可切換版本而不改腳本。
3. `persistent-data`、`staging`、`logs` 必須位於 shared runtime roots，避免 release 切換時覆寫狀態。

### 6.4 Staging 規則

1. 韌體映像先上傳至 staging area。
2. 驗證成功後才允許設定為 pending slot。
3. staging file 名稱需帶版本或 digest 資訊，避免覆寫混淆。

## 7. Log、證據與清理策略

必須保留的摘要產物：

- OBC runtime logs
- simulator logs
- GDS interaction summaries
- 測試紀錄與人工驗證證據

管理原則：

1. 不要求保存所有原始 stdout。
2. 需保留足以審查的摘要證據。
3. log 區需有 rotation 或清理策略，避免長期填滿 SD card。

## 8. 監控與告警

| 項目 | 指標 | 建議告警 |
|---|---|---|
| OBC RSS | `SYS_MEM_RSS_MB` | > 160 MB |
| CPU 使用率 | `SYS_CPU_USAGE` | > 85% 持續 60 秒 |
| CSP free buffers | `CSP_FREE_BUFFERS` | < 16 |
| staging 剩餘空間 | storage monitor | < 2 倍映像大小 |
| log 區容量 | storage monitor | > 80% |

行動原則：

- warning：記錄事件並持續運作
- alarm：限制非必要功能或切換低負載模式
- fatal：僅在可能造成資料損毀或無法恢復時觸發

## 9. 驗證狀態術語

- `Blocked-HW`：缺少實體 UART、真實 radio、GPIO / I2C 或真實儲存介面而無法完成驗證
- `Deferred-RPi`：需等 Raspberry Pi 目標整合完成後才能進行的驗證

每個受限項目至少需附一種替代證據：

1. TCP mock 或 PTY 模擬結果
2. L1 / L2 測試結果
3. GDS / script 驗證摘要
4. 對應 code path 的靜態檢查說明

## 10. 驗收錨點

1. 資源描述完全以 Linux / filesystem 模型表達。
2. 具備 slot A / B、staging、persistent、logs 的清楚邊界。
3. Boot metadata 明確為 file-backed storage，不引入資料庫。
4. 有明確監控指標與證據保存原則。
