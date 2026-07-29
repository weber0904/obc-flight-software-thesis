module OBC {

  passive component EpsBridge {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    output port epsStatusRefreshTlmOut: Fw.Tlm
    time get port Time

    sync input port schedIn: Svc.Sched
    output port watchdogBeatOut: Svc.WatchDog

    sync command EPS_GET_STATUS() opcode 0x00
    sync command EPS_SET_PDU(channel: U8, enabled: bool) opcode 0x01
    sync command EPS_SET_HEATER(enabled: bool) opcode 0x02
    sync command EPS_RESET() opcode 0x03

    event EPS_STATUS_RECEIVED \
      severity activity low \
      id 0x00 \
      format "EPS status updated"

    event EPS_LOW_BATTERY(soc: F32) \
      severity warning high \
      id 0x01 \
      format "EPS low battery SOC {}"

    event EPS_CRITICAL_BATTERY(soc: F32) \
      severity warning high \
      id 0x02 \
      format "EPS critical battery SOC {}"

    event EPS_OVERTEMP(tempBat: F32, threshold: F32) \
      severity warning high \
      id 0x03 \
      format "EPS battery over-temperature {} threshold {}"

    event EPS_PDU_CHANGE(pduStatus: U8) \
      severity activity high \
      id 0x04 \
      format "EPS PDU state changed to {}"

    event EPS_COMM_ERROR(code: U32) \
      severity warning high \
      id 0x05 \
      format "EPS transport error {}"

    telemetry EPS_VBAT: F32 id 0x00
    telemetry EPS_IBAT: F32 id 0x01
    telemetry EPS_SOC: F32 id 0x02
    telemetry EPS_VSOLAR: F32 id 0x03
    telemetry EPS_ISOLAR: F32 id 0x04
    telemetry EPS_TEMP_BAT: F32 id 0x05
    telemetry EPS_PDU_STATUS: U8 id 0x06 update on change
    telemetry EPS_POWER_OUT: F32 id 0x07
    telemetry EPS_HEATER_ENABLED: bool id 0x08 update on change
    telemetry EPS_OVERCURRENT_FLAGS: U8 id 0x09 update on change

  }

}
