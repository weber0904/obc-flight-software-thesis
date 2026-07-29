# 06 — Boot / Update A/B 策略

> 本文件是 narrative source document。正式主規格位於 [`openspec/specs/boot-update/spec.md`](../openspec/specs/boot-update/spec.md)。

## 1. 文件角色

本文件定義 Linux / Raspberry Pi 3B+ 導向的 Boot / Update A/B 更新流程、`BootManager` 契約、metadata 與回滾策略。

## 2. 來源對照

| legacy 文件 | 本文件承接內容 |
|---|---|
| `archive/legacy-v0/07_boot_update_manager.md` | A/B 流程、metadata、verify / activate / confirm / rollback |
| `archive/legacy-v0/02_command_telemetry_design.md` | `BOOT_*` command / telemetry / event |
| `archive/legacy-v0/03_memory_resource_design.md` | staging、persistent data、logs / evidence 分區角色 |

## 3. 邊界與排除

本文件涵蓋：

- `BootManager` F' 元件介面
- staging file 驗證流程
- slot metadata 與 A/B 切換策略
- confirm / rollback 策略

本文件不涵蓋：

- Raspberry Pi ROM / firmware bootloader 細節
- Linux 發行版特定 bootloader 寫法
- secure boot chain 與安全晶片

## 4. 儲存與 metadata 基線

邏輯角色如下：

| 項目 | 說明 |
|---|---|
| Slot A | 已知穩定版本或待切換版本 |
| Slot B | 已知穩定版本或待切換版本 |
| Staging Area | 上傳完成後待驗證映像 |
| Persistent Data | file-backed metadata 與狀態 |

Boot metadata v1 固定採 persistent data directory 下的 **檔案式儲存**，不使用資料庫。

至少保存：

- `active_slot`
- `pending_slot`
- `last_known_good_slot`
- `confirmed`
- `expected_digest`
- `last_boot_attempt_time`
- `last_error_code`

## 5. 更新資料路徑

第一版正式路徑如下：

1. 地面端以 file uplink 或等價工具將映像放入 staging area。
2. `BOOT_PREPARE_UPDATE` 登錄映像大小與 digest。
3. `BOOT_VERIFY_STAGED_IMAGE` 驗證 staging file。
4. 驗證成功後，以 `BOOT_ACTIVATE_STAGED_IMAGE` 設為下次啟動目標。
5. 重啟進入 pending slot。
6. 新版本於確認期限內送出 `BOOT_CONFIRM`。
7. 若未確認，系統回滾至 last known good slot。

映像 digest 規則：

- boot / update staged image 固定使用 `SHA-256`
- digest 採小寫 hex 字串表示
- 第一版不加入 signature 驗證
- 通訊層或 framing 的 CRC-32 不作為 staged image 驗證替代

明確禁止：

- `BOOT_WRITE_CHUNK(offset, data)`
- 以 command payload 傳整個 binary chunk
- 以 raw flash address 當作應用層介面

## 6. 公開契約

### 6.1 Commands

| 指令名稱 | 說明 |
|---|---|
| `BOOT_STATUS` | 查詢 active / pending / confirmed 狀態 |
| `BOOT_PREPARE_UPDATE` | 登錄待驗證映像 metadata |
| `BOOT_VERIFY_STAGED_IMAGE` | 驗證 staging file |
| `BOOT_ACTIVATE_STAGED_IMAGE` | 設定下次啟動 slot |
| `BOOT_CONFIRM` | 啟動後確認當前版本正常 |
| `BOOT_ROLLBACK` | 主動回滾到已知穩定版本 |

`BOOT_PREPARE_UPDATE` 的 `digest` 參數固定為 `64` 字元的小寫 SHA-256 hex。

### 6.2 Telemetry

- `BOOT_ACTIVE_SLOT`
- `BOOT_PENDING_SLOT`
- `BOOT_CONFIRMED`
- `BOOT_UPDATE_PROGRESS`
- `BOOT_LAST_ERROR`

### 6.3 Events

- `BOOT_UPDATE_PREPARED`
- `BOOT_STAGE_VERIFY_OK`
- `BOOT_STAGE_VERIFY_FAIL`
- `BOOT_SLOT_SWITCHED`
- `BOOT_CONFIRMED`
- `BOOT_ROLLBACK_TRIGGERED`

## 7. 確認與回滾策略

### 7.1 `BOOT_CONFIRM`

- 預設 timeout 為 `60` 秒
- 可配置範圍為 `30` 至 `180` 秒
- timeout 期間可搭配 health check / watchdog 作為補充判定

### 7.2 回滾條件

任一條件成立即可回滾：

- 新版本未在 timeout 內確認
- 啟動後健康檢查連續失敗
- metadata 損毀或 slot 驗證失效
- 操作人員手動送出 `BOOT_ROLLBACK`

## 8. 驗證與狀態標記

可在現況完成的驗證：

- staging verify
- activate / confirm 流程
- 未確認 timeout 觸發 rollback
- Raspberry Pi 原生建置與目標端啟動
- Raspberry Pi 目標端 process restart 後的 pending confirm / rollback 狀態延續
- Raspberry Pi 目標端 OS reboot 後，透過受管 service 路徑重新拉起 installed `current` release，且 `BOOT_STATUS` 仍可觀察到 file-backed metadata 狀態
- F' telemetry / event path

不可在現況完成的驗證：

- 真實 Raspberry Pi bootloader / partition handoff
- 真實 SD card A/B 切換與斷電測試
- 實體 UART / radio hardware path 與相關線材情境

狀態標記：

- 需等待硬體與儲存介面條件的項目標記為 `Blocked-HW`
- `Deferred-RPi` 只保留給尚未進入目標整合階段的新項目；native target build / launch / restart 類項目已由 `rpi-target-integration-v1` 清除
