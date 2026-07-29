module OBC {

  enum StorageRootKind : U8 {
    PERSISTENT = 0
    STAGING = 1
    LOGS = 2
    DATA_PRODUCTS = 3
  }

  passive component StorageHealthBridge {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    sync command STORAGE_GET_STATUS() opcode 0x00

    event STORAGE_SCAN_UPDATED(warningActive: U8, warningMask: U8) \
      severity activity low \
      id 0x00 \
      format "Storage scan updated warning={} mask={}"

    event STORAGE_ROOT_MISSING(root: StorageRootKind) \
      severity warning low \
      id 0x01 \
      format "Storage root missing {}"

    event STORAGE_SCAN_FAILED(root: StorageRootKind, code: U32) \
      severity warning high \
      id 0x02 \
      format "Storage scan failed root {} code {}"

    event STORAGE_WARNING_THRESHOLD_EXCEEDED(root: StorageRootKind, totalBytes: U32, thresholdBytes: U32) \
      severity warning high \
      id 0x03 \
      format "Storage warning root {} total {} threshold {}"

    telemetry STORAGE_HAVE_SCAN: U8 id 0x00
    telemetry STORAGE_WARNING_ACTIVE: U8 id 0x01
    telemetry STORAGE_WARNING_MASK: U8 id 0x02
    telemetry STORAGE_DEGRADED_MASK: U8 id 0x03
    telemetry STORAGE_SCAN_COUNT: U32 id 0x04
    telemetry STORAGE_SCAN_ERRORS: U32 id 0x05
    telemetry STORAGE_PERSISTENT_FILE_COUNT: U32 id 0x06
    telemetry STORAGE_PERSISTENT_BYTES: U32 id 0x07
    telemetry STORAGE_STAGING_FILE_COUNT: U32 id 0x08
    telemetry STORAGE_STAGING_BYTES: U32 id 0x09
    telemetry STORAGE_LOG_FILE_COUNT: U32 id 0x0A
    telemetry STORAGE_LOG_BYTES: U32 id 0x0B
    telemetry STORAGE_DATA_PRODUCTS_EXISTS: U8 id 0x0C
    telemetry STORAGE_DATA_PRODUCTS_SCAN_OK: U8 id 0x0D
    telemetry STORAGE_DATA_PRODUCTS_FILE_COUNT: U32 id 0x0E
    telemetry STORAGE_DATA_PRODUCTS_BYTES: U32 id 0x0F
    telemetry STORAGE_DATA_PRODUCTS_ERROR_CODE: U32 id 0x10
    telemetry STORAGE_DATA_PRODUCTS_QUOTA_BYTES: U32 id 0x11
    telemetry STORAGE_DATA_PRODUCTS_WATERMARK_BYTES: U32 id 0x12
    telemetry STORAGE_DATA_PRODUCTS_QUOTA_STATUS: U8 id 0x13
    telemetry STORAGE_DATA_PRODUCTS_RETENTION_STATUS: U8 id 0x14
    telemetry STORAGE_SCHED_TICKS: U32 id 0x15
  }

}
