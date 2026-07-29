module OBC {

  enum HkTrendFlushReason : U8 {
    SIZE_THRESHOLD = 0
    MANUAL_FLUSH = 1
    THRESHOLD_CHANGE = 2
  }

  struct HkTrendChunkMetaV1 {
    version: U16
    chunkSequence: U32
    firstSampleSequence: U32
    lastSampleSequence: U32
    sampleCount: U32
    targetFileBytes: U32
    flushReason: HkTrendFlushReason
  }

  struct HkTrendRecordV6 {
    version: U16
    sequence: U32
    timeBase: U32
    timeContext: U32
    timeSeconds: U32
    timeUSeconds: U32
    mode: SatMode
    uptimeSec: U32
    rebootCount: U16
    activeBootSlot: BootSlot
    lastResetReason: U8
    haveEpsStatus: bool
    epsVbat: F32
    epsIbat: F32
    epsSoc: F32
    epsVSolar: F32
    epsISolar: F32
    epsTempBat: F32
    epsPowerOut: F32
    epsPduStatus: U8
    epsSunlight: U8
    epsHeaterEnabled: U8
    epsOvercurrentFlags: U8
    haveAdcsState: bool
    adcsMode: U8
    adcsSensorValid: U8
    adcsRateNorm: F32
    adcsQ0: F64
    adcsQ1: F64
    adcsQ2: F64
    adcsQ3: F64
    adcsOmegaX: F32
    adcsOmegaY: F32
    adcsOmegaZ: F32
    adcsMagX: F32
    adcsMagY: F32
    adcsMagZ: F32
    adcsPointingErrorDeg: F32
    haveGpsState: bool
    gpsHasSample: bool
    gpsFixValid: bool
    gpsSourceMode: U8
    gpsLatitudeDeg: F64
    gpsLongitudeDeg: F64
    gpsAltitudeMeters: F32
    gpsSpeedMetersPerSecond: F32
    gpsCourseDegrees: F32
    gpsSatelliteCount: U8
    gpsHdop: F32
    gpsUtcSecondsOfDay: U32
    gpsUtcDateYmd: U32
    gpsAcceptedSentences: U32
    gpsRejectedSentences: U32
    haveStorageHealth: bool
    storageWarningActive: bool
    storageWarningMask: U8
    storageDegradedMask: U8
    storageScanCount: U32
    storageScanErrorCount: U32
    storagePersistentExists: bool
    storagePersistentScanOk: bool
    storagePersistentFileCount: U32
    storagePersistentBytes: U32
    storagePersistentErrorCode: U32
    storagePersistentQuotaBytes: U32
    storagePersistentWatermarkBytes: U32
    storagePersistentQuotaStatus: U8
    storagePersistentRetentionStatus: U8
    storageStagingExists: bool
    storageStagingScanOk: bool
    storageStagingFileCount: U32
    storageStagingBytes: U32
    storageStagingErrorCode: U32
    storageStagingQuotaBytes: U32
    storageStagingWatermarkBytes: U32
    storageStagingQuotaStatus: U8
    storageStagingRetentionStatus: U8
    storageLogsExists: bool
    storageLogsScanOk: bool
    storageLogsFileCount: U32
    storageLogsBytes: U32
    storageLogsErrorCode: U32
    storageLogsQuotaBytes: U32
    storageLogsWatermarkBytes: U32
    storageLogsQuotaStatus: U8
    storageLogsRetentionStatus: U8
    storageDataProductsExists: bool
    storageDataProductsScanOk: bool
    storageDataProductsFileCount: U32
    storageDataProductsBytes: U32
    storageDataProductsErrorCode: U32
    storageDataProductsQuotaBytes: U32
    storageDataProductsWatermarkBytes: U32
    storageDataProductsQuotaStatus: U8
    storageDataProductsRetentionStatus: U8
    commActiveBand: CommBand
    commPassActive: bool
    commPassRemainingSec: U32
    commTotalPasses: U32
    cspInitialized: bool
    cspLocalNodeId: U8
    cspTxPackets: U32
    cspRxPackets: U32
    cspErrorCount: U32
    cspFreeBuffers: U32
    uartConnected: bool
    uartTxBytes: U32
    uartRxBytes: U32
    uartTxErrors: U32
    uartRxErrors: U32
    radioLinkConnected: bool
    haveRadioStatus: bool
    radioEnabled: bool
    radioPowerDbm: U8
    radioFreqHz: U32
    radioTemperatureC: F32
    radioRssiDbm: I16
    radioTxBytes: U32
    radioRxBytes: U32
    radioTxErrors: U32
    radioRxErrors: U32
    pendingBootSlot: BootSlot
    lastKnownGoodBootSlot: BootSlot
    bootConfirmed: bool
    bootStageVerified: bool
    bootExpectedSize: U32
    bootLastBootAttemptTime: U32
    bootLastErrorCode: U32
    bootRemainingConfirmSeconds: U32
    bootUpdateProgress: U8
    healthMask: U32
    faultMask: U32
    qualityMask: U32
  }

  passive component HkTrendProductProducer {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    param get port prmGetOut
    param set port prmSetOut
    time get port Time

    sync input port schedIn: Svc.Sched

    product get port productGetOut
    product send port productSendOut
    output port productBufferReturnOut: Fw.BufferSend

    sync command HK_TREND_FLUSH() opcode 0x00
    sync command HK_TREND_GET_STATUS() opcode 0x01

    param HK_TREND_TARGET_FILE_BYTES: U32 default 8192 id 0x00

    product record HkTrendRecord: HkTrendRecordV6 array id 0
    product record HkTrendChunkMeta: HkTrendChunkMetaV1 id 1
    product container HkTrendContainer id 0x01 default priority 20

    event HK_TREND_PRODUCT_WRITTEN(chunkSequence: U32, firstSampleSequence: U32, lastSampleSequence: U32, sampleCount: U32, sizeBytes: U32, flushReason: HkTrendFlushReason) \
      severity activity low \
      id 0x00 \
      format "HK trend data product written chunk {} first {} last {} count {} size {} reason {}"

    event HK_TREND_SOURCE_UNAVAILABLE \
      severity warning high \
      id 0x01 \
      format "HK trend state source unavailable"

    event HK_TREND_PRODUCT_REJECTED(sequence: U32, code: U32) \
      severity warning high \
      id 0x02 \
      format "HK trend data product rejected sequence {} code {}"

    event HK_TREND_STATUS(targetFileBytes: U32, pendingSampleCount: U32, pendingEstimatedBytes: U32, nextSampleSequence: U32, nextChunkSequence: U32) \
      severity activity low \
      id 0x03 \
      format "HK trend status target {} pendingSamples {} pendingBytes {} nextSample {} nextChunk {}"

    telemetry HK_TREND_PRODUCT_COUNT: U32 id 0x00
    telemetry HK_TREND_LAST_SEQUENCE: U32 id 0x01
    telemetry HK_TREND_LAST_SIZE_BYTES: U32 id 0x02
    telemetry HK_TREND_LAST_ERROR: U32 id 0x03 update on change
    telemetry HK_TREND_LAST_CHUNK_SEQUENCE: U32 id 0x04
    telemetry HK_TREND_PENDING_SAMPLE_COUNT: U32 id 0x05
    telemetry HK_TREND_PENDING_ESTIMATED_BYTES: U32 id 0x06
    telemetry HK_TREND_TARGET_FILE_BYTES: U32 id 0x07 update on change
  }
}
