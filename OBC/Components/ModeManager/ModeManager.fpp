module OBC {

  passive component ModeManager {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    output port modeGetTlmOut: Fw.Tlm
    time get port Time

    sync command MODE_SET(mode: SatMode) opcode 0x00
    sync command MODE_GET() opcode 0x01

    event SYS_BOOT \
      severity activity high \
      id 0x00 \
      format "System booted"

    event SYS_MODE_CHANGE(mode: SatMode) \
      severity activity high \
      id 0x01 \
      format "System mode changed to {}"

    event SYS_MODE_TRANSITION_REJECTED(fromMode: SatMode, toMode: SatMode, reasonCode: U32) \
      severity warning high \
      id 0x02 \
      format "System mode transition rejected from {} to {} reason {}"

    telemetry SYS_MODE: SatMode id 0x00 update on change
    telemetry SYS_UPTIME_SEC: U32 id 0x01
    telemetry SYS_REBOOT_COUNT: U16 id 0x02 update on change

  }

}
