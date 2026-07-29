module OBC {

  passive component AdcsBridge {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    output port adcsStatusRefreshTlmOut: Fw.Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    sync command ADCS_SET_MODE(mode: AdcsMode) opcode 0x00
    sync command ADCS_SET_TARGET(q0: F64, q1: F64, q2: F64, q3: F64) opcode 0x01
    sync command ADCS_GET_ATTITUDE() opcode 0x02
    sync command ADCS_CALIBRATE(sensorId: U8) opcode 0x03

    event ADCS_MODE_CHANGE(mode: AdcsMode) \
      severity activity high \
      id 0x00 \
      format "ADCS mode changed to {}"

    event ADCS_DETUMBLE_COMPLETE(rateNorm: F32) \
      severity activity high \
      id 0x01 \
      format "ADCS detumble complete with rate {}"

    event ADCS_POINTING_ACQUIRED(pointingError: F32) \
      severity activity high \
      id 0x02 \
      format "ADCS pointing acquired with error {} deg"

    event ADCS_SENSOR_FAULT(code: U32) \
      severity warning high \
      id 0x03 \
      format "ADCS sensor fault {}"

    event ADCS_COMM_ERROR(code: U32) \
      severity warning high \
      id 0x04 \
      format "ADCS transport error {}"

    telemetry ADCS_Q0: F64 id 0x00
    telemetry ADCS_Q1: F64 id 0x01
    telemetry ADCS_Q2: F64 id 0x02
    telemetry ADCS_Q3: F64 id 0x03
    telemetry ADCS_OMEGA_X: F32 id 0x04
    telemetry ADCS_OMEGA_Y: F32 id 0x05
    telemetry ADCS_OMEGA_Z: F32 id 0x06
    telemetry ADCS_MAG_X: F32 id 0x07
    telemetry ADCS_MAG_Y: F32 id 0x08
    telemetry ADCS_MAG_Z: F32 id 0x09
    telemetry ADCS_MODE: AdcsMode id 0x0A
    telemetry ADCS_POINTING_ERR: F32 id 0x0B

  }

}
