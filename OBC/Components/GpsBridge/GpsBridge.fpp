module OBC {

  passive component GpsBridge {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    sync command GPS_GET_STATE() opcode 0x00
    sync command GPS_SET_SOURCE_MODE(mode: GpsSourceMode) opcode 0x01

    event GPS_STATE_UPDATED(fixValid: U8, satelliteCount: U8) \
      severity activity low \
      id 0x00 \
      format "GPS state updated fix={} satellites={}"

    event GPS_FIX_ACQUIRED(latitudeDeg: F64, longitudeDeg: F64) \
      severity activity high \
      id 0x01 \
      format "GPS fix acquired lat={} lon={}"

    event GPS_FIX_LOST \
      severity warning low \
      id 0x02 \
      format "GPS fix lost"

    event GPS_PARSE_ERROR(code: U32) \
      severity warning high \
      id 0x03 \
      format "GPS parse error {}"

    event GPS_SOURCE_ERROR(code: U32) \
      severity warning high \
      id 0x04 \
      format "GPS source error {}"

    telemetry GPS_SOURCE_MODE: GpsSourceMode id 0x00
    telemetry GPS_HAVE_SAMPLE: U8 id 0x01
    telemetry GPS_FIX_VALID: U8 id 0x02
    telemetry GPS_LAT_DEG: F64 id 0x03
    telemetry GPS_LON_DEG: F64 id 0x04
    telemetry GPS_ALT_M: F32 id 0x05
    telemetry GPS_SPEED_MPS: F32 id 0x06
    telemetry GPS_COURSE_DEG: F32 id 0x07
    telemetry GPS_SAT_COUNT: U8 id 0x08
    telemetry GPS_HDOP: F32 id 0x09
    telemetry GPS_UTC_SEC_OF_DAY: U32 id 0x0A
    telemetry GPS_UTC_DATE_YMD: U32 id 0x0B
    telemetry GPS_ACCEPTED_SENTENCES: U32 id 0x0C
    telemetry GPS_REJECTED_SENTENCES: U32 id 0x0D

  }

}
