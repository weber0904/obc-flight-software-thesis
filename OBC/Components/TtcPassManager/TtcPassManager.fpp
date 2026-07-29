module OBC {

  passive component TtcPassManager {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    sync command TTC_SET_POLICY(enabled: bool, lossOfLockTimeoutSec: U32) opcode 0x00
    sync command TTC_SET_PASS_WINDOW(startUnixSec: U64, endUnixSec: U64) opcode 0x01
    sync command TTC_CLEAR_PASS_WINDOW() opcode 0x02
    sync command TTC_GET_STATUS() opcode 0x03

    event TTC_POLICY_CONFIG_UPDATED(enabled: U8, lossOfLockTimeoutSec: U32) \
      severity activity low \
      id 0x00 \
      format "TTC policy config updated enabled {} loss timeout {}"

    event TTC_PASS_WINDOW_SET(startUnixSec: U64, endUnixSec: U64) \
      severity activity low \
      id 0x01 \
      format "TTC pass window set start {} end {}"

    event TTC_PASS_WINDOW_CLEARED \
      severity activity low \
      id 0x02 \
      format "TTC pass window cleared"

    event TTC_PASS_WINDOW_REJECTED(startUnixSec: U64, endUnixSec: U64) \
      severity warning high \
      id 0x03 \
      format "TTC pass window rejected start {} end {}"

    event TTC_POLICY_ENTRY_REQUEST(reasonCode: U32) \
      severity activity high \
      id 0x04 \
      format "TTC policy requested TTC entry reason {}"

    event TTC_POLICY_EXIT_REQUEST(reasonCode: U32) \
      severity activity high \
      id 0x05 \
      format "TTC policy requested TTC exit reason {}"

    event TTC_POLICY_ADCS_POINTING_REQUEST_FAILED \
      severity warning high \
      id 0x06 \
      format "TTC policy entry could not trigger ADCS pointing"

    telemetry TTC_POLICY_ENABLED: bool id 0x00
    telemetry TTC_POLICY_LOSS_TIMEOUT_SEC: U32 id 0x01
    telemetry TTC_POLICY_WINDOW_CONFIGURED: bool id 0x02
    telemetry TTC_POLICY_WINDOW_START_UNIX_SEC: U64 id 0x03
    telemetry TTC_POLICY_WINDOW_END_UNIX_SEC: U64 id 0x04
    telemetry TTC_POLICY_WINDOW_ACTIVE: bool id 0x05
    telemetry TTC_POLICY_GPS_TIME_VALID: bool id 0x06
    telemetry TTC_POLICY_CURRENT_GPS_UNIX_SEC: U64 id 0x07
    telemetry TTC_POLICY_TTC_ACTIVE: bool id 0x08
    telemetry TTC_POLICY_LOSS_TIMER_SEC: U32 id 0x09
    telemetry TTC_POLICY_LAST_ENTRY_REASON: U32 id 0x0A
    telemetry TTC_POLICY_LAST_EXIT_REASON: U32 id 0x0B
    telemetry TTC_POLICY_ENTRY_COUNT: U32 id 0x0C
    telemetry TTC_POLICY_EXIT_COUNT: U32 id 0x0D

  }

}
