module OBC {

  passive component RadioController {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    sync command RADIO_ENABLE(enable: bool) opcode 0x00
    sync command RADIO_SET_POWER(powerDbm: U8) opcode 0x01
    sync command RADIO_SET_FREQ(freqHz: U32) opcode 0x02
    sync command RADIO_GET_STATUS() opcode 0x03

    event RADIO_POWERED_ON \
      severity activity high \
      id 0x00 \
      format "Radio powered on"

    event RADIO_POWERED_OFF \
      severity activity high \
      id 0x01 \
      format "Radio powered off"

    event RADIO_OVERTEMP(tempC: F32) \
      severity warning high \
      id 0x02 \
      format "Radio over-temperature {} C"

    telemetry RADIO_ENABLED: bool id 0x00 update on change
    telemetry RADIO_TX_POWER: U8 id 0x01
    telemetry RADIO_FREQ: U32 id 0x02
    telemetry RADIO_TEMP: F32 id 0x03
    telemetry RADIO_RSSI: I16 id 0x04
    telemetry RADIO_STATUS_SAMPLE_AVAILABLE: bool id 0x05 update on change
    telemetry RADIO_STATUS_AGE_TICKS: U32 id 0x06
    telemetry RADIO_STATUS_RESULT: U32 id 0x07 update on change

  }

}
