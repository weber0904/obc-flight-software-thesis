module OBC {

  passive component LinuxWatchdogSink {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port watchdogFeedIn: Svc.WatchDog
    sync command GET_HW_WATCHDOG_STATUS() opcode 0x00

    event HW_WATCHDOG_CONFIG_UPDATED(enabled: bool, timeoutSec: U32) \
      severity activity high \
      id 0x00 \
      format "Hardware watchdog enabled {} timeout {} sec"

    event HW_WATCHDOG_OPENED(timeoutSec: U32) \
      severity activity high \
      id 0x01 \
      format "Hardware watchdog device opened timeout {} sec"

    event HW_WATCHDOG_CLOSED() \
      severity activity high \
      id 0x02 \
      format "Hardware watchdog device closed"

    event HW_WATCHDOG_OPEN_FAILED(errorCode: U32) \
      severity warning high \
      id 0x03 \
      format "Hardware watchdog open failed error {}"

    event HW_WATCHDOG_KEEPALIVE_FAILED(errorCode: U32) \
      severity warning high \
      id 0x04 \
      format "Hardware watchdog keepalive failed error {}"

    event HW_WATCHDOG_STATUS(enabled: bool, deviceOpen: bool, timeoutSec: U32, feedCount: U32, lastError: U32) \
      severity activity high \
      id 0x05 \
      format "Hardware watchdog status enabled {} open {} timeout {} feedCount {} lastError {}"

    telemetry HW_WATCHDOG_ENABLED: bool id 0x00 update on change
    telemetry HW_WATCHDOG_DEVICE_OPEN: bool id 0x01 update on change
    telemetry HW_WATCHDOG_TIMEOUT_SEC: U32 id 0x02
    telemetry HW_WATCHDOG_FEED_COUNT: U32 id 0x03
    telemetry HW_WATCHDOG_LAST_FEED_CODE: U32 id 0x04
    telemetry HW_WATCHDOG_LAST_ERROR: U32 id 0x05

  }

}
