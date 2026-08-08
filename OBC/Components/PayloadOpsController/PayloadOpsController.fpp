module OBC {

  enum PayloadState : U8 {
    PSTATE_OFF = 0
    PSTATE_PREPARING = 1
    PSTATE_READY = 2
    PSTATE_CAPTURING = 3
    PSTATE_PUBLISHING = 4
    PSTATE_ABORTING = 5
    PSTATE_FAULT = 6
  }

  enum PayloadReadyKind : U8 {
    READY_NON_RAW = 0
    READY_RAW_SENSOR = 1
  }

  enum PayloadCapturePolicy : U8 {
    CAPTURE_DETERMINISTIC = 0
    CAPTURE_AUTO = 1
    CAPTURE_RAW_SENSOR = 2
  }

  enum PayloadArtifactKind : U8 {
    PREVIEW_JPEG = 0
    RAW_FRAME = 1
  }

  enum PayloadPixelFormat : U8 {
    PIXEL_UNKNOWN = 0
    PIXEL_YUYV = 1
    PIXEL_UYVY = 2
    PIXEL_NV12 = 3
    PIXEL_RGB888 = 4
    PIXEL_BGR888 = 5
  }

  enum PayloadResolutionPreset : U8 {
    PRESET_VGA_640X480 = 0
    PRESET_HD_1280X720 = 1
    PRESET_FULL_3280X2464 = 2
  }

  enum PayloadAwbMode : U8 {
    AWB_AUTO = 0
    AWB_OFF = 1
    AWB_DAYLIGHT = 2
    AWB_CLOUDY = 3
    AWB_TUNGSTEN = 4
    AWB_FLUORESCENT = 5
  }

  enum PayloadMeteringMode : U8 {
    METER_CENTRE = 0
    METER_SPOT = 1
    METER_MATRIX = 2
  }

  enum PayloadResultCode : U32 {
    PRESULT_NONE = 0
    PRESULT_OK = 1
    PRESULT_REJECTED_MODE = 2
    PRESULT_REJECTED_BUSY = 3
    PRESULT_REJECTED_NOT_READY = 4
    PRESULT_REJECTED_INVALID_CONFIG = 5
    PRESULT_REJECTED_UNCONFIGURED = 6
    PRESULT_PROXY_POWER_FAILED = 7
    PRESULT_PREPARE_FAILED = 8
    PRESULT_CAPTURE_FAILED = 9
    PRESULT_SHUTDOWN_FAILED = 10
    PRESULT_ABORTED = 11
    PRESULT_MODE_EXIT_ABORTED = 12
    PRESULT_STORAGE_FAILED = 13
    PRESULT_REJECTED_UNSUPPORTED = 14
  }

  struct PayloadCaptureArtifactHeaderV2 {
    version: U16
    captureId: U32
    captureIndex: U8
    artifactKind: PayloadArtifactKind
    pixelFormat: PayloadPixelFormat
    reserved0: U8
    bootCount: U32
    captureTimeSec: U32
    captureTimeUsec: U32
    capturePolicy: PayloadCapturePolicy
    resultCode: PayloadResultCode
    requestedMask: U32
    appliedMask: U32
    resolution: PayloadResolutionPreset
    jpegQualityApplied: U32
    exposureUsec: U32
    gainX100: U32
    actualExposureUsec: U32
    actualGainX100: U32
    actualAwbValid: bool
    actualAwbColorTemperatureK: U32
    actualAwbRedGainX1000: U32
    actualAwbBlueGainX1000: U32
    imageWidth: U32
    imageHeight: U32
    rawBytes: U32
    previewJpegBytes: U32
    backendName: string size 64
    cameraModel: string size 64
    rawRelativePath: string size 255
    previewRelativePath: string size 255
    previewDataProductRelativePath: string size 255
    rawDataProductRelativePath: string size 255
    previewDataProductPublished: bool
    rawDataProductPublished: bool
  }

  passive component PayloadOpsController {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time
    sync input port dpWrittenIn: Svc.DpWritten

    product get port productGetOut
    product send port productSendOut
    output port productBufferReturnOut: Fw.BufferSend

    product record PayloadCaptureArtifactHeader: PayloadCaptureArtifactHeaderV2 id 0
    product record PayloadCaptureArtifactBytes: U8 array id 1
    product container PayloadCaptureArtifactContainer id 0x01 default priority 25

    sync input port schedIn: Svc.Sched

    sync command PAYLOAD_SET_CAMERA_DEFAULTS(
      resolutionPreset: PayloadResolutionPreset,
      jpegQuality: U32,
      hflip: bool,
      vflip: bool
    ) opcode 0x00
    sync command PAYLOAD_PREPARE() opcode 0x01
    sync command PAYLOAD_ABORT() opcode 0x03
    sync command PAYLOAD_SHUTDOWN() opcode 0x04
    sync command PAYLOAD_GET_STATUS() opcode 0x05

    sync command PAYLOAD_PREPARE_RAW_SENSOR() opcode 0x06
    sync command PAYLOAD_SET_AUTO_DEFAULTS(
      awbMode: PayloadAwbMode,
      meteringMode: PayloadMeteringMode,
      evCompX100: I32
    ) opcode 0x07
    sync command PAYLOAD_SET_DETERMINISTIC_DEFAULTS(
      exposureUsec: U32,
      gainX100: U32
    ) opcode 0x08
    sync command PAYLOAD_CAPTURE_AUTO(
      captureIndex: U8,
      tag: string size 64,
      applyMask: U32,
      awbMode: PayloadAwbMode,
      meteringMode: PayloadMeteringMode,
      evCompX100: I32
    ) opcode 0x09
    sync command PAYLOAD_CAPTURE_DETERMINISTIC(
      captureIndex: U8,
      tag: string size 64,
      applyMask: U32,
      exposureUsec: U32,
      gainX100: U32
    ) opcode 0x0A
    sync command PAYLOAD_GET_CAPABILITIES() opcode 0x0B
    sync command PAYLOAD_GET_LAST_CAPTURE_METADATA() opcode 0x0C
    sync command PAYLOAD_SENSOR_REG_READ(address: U32) opcode 0x0D
    sync command PAYLOAD_SENSOR_REG_WRITE(address: U32, value: U32, verifyReadback: bool) opcode 0x0E
    sync command PAYLOAD_CAPTURE_RAW(captureIndex: U8, tag: string size 64) opcode 0x0F
    sync command PAYLOAD_PUBLISH_CAPTURE(captureIndex: U8, artifactKind: PayloadArtifactKind) opcode 0x10

    event PAYLOAD_STATE_CHANGED(payloadState: PayloadState, resultCode: PayloadResultCode) \
      severity activity high \
      id 0x00 \
      format "Payload state {} result {}"

    event PAYLOAD_CAPTURED(captureId: U32, captureIndex: U8, rawRelativePath: string size 255, previewRelativePath: string size 255) \
      severity activity high \
      id 0x01 \
      format "Payload captured {} index {} raw {} preview {}"

    event PAYLOAD_OPERATION_FAILED(resultCode: PayloadResultCode, detail: U32) \
      severity warning high \
      id 0x02 \
      format "Payload operation failed result {} detail {}"

    event PAYLOAD_PROXY_POWER_CHANGED(enabled: bool, channel: U32) \
      severity activity low \
      id 0x03 \
      format "Payload proxy power {} channel {}"

    event PAYLOAD_STATUS(payloadState: PayloadState, powered: bool, prepared: bool, lastResult: PayloadResultCode, captureId: U32, captureIndex: U8, previewRelativePath: string size 255, previewDataProductPath: string size 255, previewPublished: bool, rawPublished: bool) \
      severity activity low \
      id 0x04 \
      format "Payload status state {} powered {} prepared {} result {} capture {} index {} preview {} previewDp {} previewPublished {} rawPublished {}"

    event PAYLOAD_CAPABILITIES(
      backendName: string size 64,
      supportedMask: U32,
      offOnlyMask: U32,
      autoMutableMask: U32,
      deterministicMutableMask: U32,
      rawRegisterSupported: bool,
      realSensorPath: bool
    ) severity activity low \
      id 0x05 \
      format "Payload capabilities backend {} supported {} offOnly {} autoMutable {} deterministicMutable {} raw {} real {}"

    event PAYLOAD_CAPTURE_METADATA(
      capturePolicy: PayloadCapturePolicy,
      captureId: U32,
      captureIndex: U8,
      requestedMask: U32,
      appliedMask: U32,
      actualExposureUsec: U32,
      actualGainX100: U32,
      actualAwbValid: bool,
      actualAwbColorTemperatureK: U32,
      actualAwbRedGainX1000: U32,
      actualAwbBlueGainX1000: U32,
      rawRelativePath: string size 255,
      previewRelativePath: string size 255,
      previewDataProductPath: string size 255,
      rawDataProductPath: string size 255,
      previewPublished: bool,
      rawPublished: bool
    ) severity activity low \
      id 0x06 \
      format "Payload capture metadata policy {} capture {} index {} requested {} applied {} actualExp {} actualGain {} awbValid {} awbTempK {} awbRedX1000 {} awbBlueX1000 {} raw {} preview {} previewDp {} rawDp {} previewPublished {} rawPublished {}"

    event PAYLOAD_SENSOR_REGISTER_VALUE(address: U32, value: U32) \
      severity activity low \
      id 0x07 \
      format "Payload sensor register {} value {}"

    event PAYLOAD_SENSOR_REGISTER_WRITTEN(address: U32, value: U32, verifiedValue: U32, verifyReadback: bool) \
      severity activity low \
      id 0x08 \
      format "Payload sensor register {} write {} verified {} verify {}"

    telemetry PAYLOAD_STATE: PayloadState id 0x00 update on change
    telemetry PAYLOAD_LOGICAL_POWERED: bool id 0x01 update on change
    telemetry PAYLOAD_PREPARED: bool id 0x02 update on change
    telemetry PAYLOAD_BUSY: bool id 0x03 update on change
    telemetry PAYLOAD_LAST_RESULT: PayloadResultCode id 0x04 update on change
    telemetry PAYLOAD_LAST_DETAIL: U32 id 0x05 update on change
    telemetry PAYLOAD_LAST_CAPTURE_ID: U32 id 0x06 update on change
    telemetry PAYLOAD_LAST_CAPTURE_INDEX: U8 id 0x19 update on change
    telemetry PAYLOAD_PROXY_CHANNEL: U32 id 0x07 update on change
    telemetry PAYLOAD_PROXY_ASSERTED: bool id 0x08 update on change
    telemetry PAYLOAD_DEFAULT_RESOLUTION: PayloadResolutionPreset id 0x09 update on change
    telemetry PAYLOAD_DEFAULT_EXPOSURE_USEC: U32 id 0x0A update on change
    telemetry PAYLOAD_DEFAULT_GAIN_X100: U32 id 0x0B update on change
    telemetry PAYLOAD_ABORT_TOTAL: U32 id 0x0C update on change
    telemetry PAYLOAD_DEFAULT_JPEG_QUALITY: U32 id 0x0D update on change
    telemetry PAYLOAD_ACTIVE_READY: PayloadReadyKind id 0x0F update on change
    telemetry PAYLOAD_LAST_CAPTURE_POLICY: PayloadCapturePolicy id 0x10 update on change
    telemetry PAYLOAD_LAST_REQUESTED_MASK: U32 id 0x11 update on change
    telemetry PAYLOAD_LAST_APPLIED_MASK: U32 id 0x12 update on change
    telemetry PAYLOAD_CAP_SUPPORTED_MASK: U32 id 0x13 update on change
    telemetry PAYLOAD_CAP_OFF_ONLY_MASK: U32 id 0x14 update on change
    telemetry PAYLOAD_CAP_AUTO_MUTABLE_MASK: U32 id 0x15 update on change
    telemetry PAYLOAD_CAP_DETERMINISTIC_MUTABLE_MASK: U32 id 0x16 update on change
    telemetry PAYLOAD_LAST_DATA_PRODUCT_PUBLISHED: bool id 0x17 update on change
    telemetry PAYLOAD_LAST_DATA_PRODUCT_BYTES: U32 id 0x18 update on change
    telemetry PAYLOAD_LAST_RAW_DATA_PRODUCT_PUBLISHED: bool id 0x1A update on change
    telemetry PAYLOAD_LAST_RAW_DATA_PRODUCT_BYTES: U32 id 0x1B update on change
    telemetry PAYLOAD_LAST_PUBLISHED_ARTIFACT_KIND: PayloadArtifactKind id 0x1C update on change

  }

}
