module OBC {

  passive component WatchdogSupervisor {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched
    sync input port beatIn: [WatchdogSupervisedSourceCount] Svc.WatchDog

    output port watchdogFeedOut: Svc.WatchDog

    sync command HEALTH_ENABLE(enable: bool) opcode 0x00
    sync command HEALTH_SET_THRESHOLD(item: HealthItem, value: F32) opcode 0x01
    sync command GET_WATCHDOG_STATUS() opcode 0x02
    sync command SET_WATCHDOG_CONFIG(wdSource: WatchdogSource, enabled: bool, warningTicks: U32, safeTicks: U32, suppressTicks: U32) opcode 0x03
    sync command SET_WATCHDOG_PROBE_SUPPRESSION(wdSource: WatchdogSource, suppressed: bool) opcode 0x04

    event HEALTH_CHECKING_SET(enabled: bool) \
      severity activity high \
      id 0x00 \
      format "Health checking enabled {}"

    event HEALTH_THRESHOLD_UPDATED(item: HealthItem, value: F32) \
      severity activity high \
      id 0x01 \
      format "Health threshold {} set to {}"

    event SYS_LOW_MEMORY(currentRssMb: F32, thresholdMb: F32) \
      severity warning high \
      id 0x02 \
      format "RSS {} MB exceeded threshold {} MB"

    event SYS_RESOURCE_DEGRADED(item: HealthItem, observed: F32, threshold: F32) \
      severity warning high \
      id 0x03 \
      format "Resource {} degraded: observed {} threshold {}"

    event WATCHDOG_SOURCE_WARNING(wdSource: WatchdogSource, ageTicks: U32, warningTicks: U32) \
      severity warning low \
      id 0x04 \
      format "Watchdog source {} warning at age {} threshold {}"

    event WATCHDOG_SOURCE_FAULT_ENTERED(wdSource: WatchdogSource, ageTicks: U32, currentMode: SatMode, requestedSafe: bool) \
      severity warning high \
      id 0x05 \
      format "Watchdog source {} fault latched at age {} mode {} requestedSafe {}"

    event WATCHDOG_SOURCE_FEED_SUPPRESSED(wdSource: WatchdogSource, ageTicks: U32, suppressTicks: U32) \
      severity warning high \
      id 0x06 \
      format "Watchdog source {} feed suppressed at age {} threshold {}"

    event WATCHDOG_SOURCE_RECOVERED(wdSource: WatchdogSource, previousState: WatchdogState) \
      severity activity high \
      id 0x07 \
      format "Watchdog source {} recovered from {}"

    event WATCHDOG_CONFIG_UPDATED(wdSource: WatchdogSource, enabled: bool, warningTicks: U32, safeTicks: U32, suppressTicks: U32) \
      severity activity high \
      id 0x08 \
      format "Watchdog config {} enabled {} warn {} safe {} suppress {}"

    event WATCHDOG_FEED_ELIGIBILITY_CHANGED(feedEligible: bool, faultMask: U32, suppressMask: U32, recoveryLevel: WatchdogRecoveryLevel) \
      severity activity high \
      id 0x09 \
      format "Watchdog feed eligible {} faultMask 0x{x} suppressMask 0x{x} recovery {}"

    event WATCHDOG_STATUS(aggregateState: WatchdogState, recoveryLevel: WatchdogRecoveryLevel, feedEligible: bool, warningMask: U32, faultMask: U32, suppressMask: U32) \
      severity activity high \
      id 0x0A \
      format "Watchdog aggregate {} recovery {} eligible {} warningMask 0x{x} faultMask 0x{x} suppressMask 0x{x}"

    event WATCHDOG_SOURCE_STATUS(wdSource: WatchdogSource, enabled: bool, wdState: WatchdogState, ageTicks: U32, warningTicks: U32, safeTicks: U32, suppressTicks: U32) \
      severity activity high \
      id 0x0B \
      format "Watchdog source {} enabled {} state {} age {} warn {} safe {} suppress {}"

    event WATCHDOG_PROBE_SUPPRESSION_UPDATED(wdSource: WatchdogSource, suppressed: bool) \
      severity activity high \
      id 0x0C \
      format "Watchdog probe suppression {} set to {}"

    telemetry SYS_CPU_USAGE: F32 id 0x00
    telemetry SYS_MEM_RSS_MB: F32 id 0x01
    telemetry WATCHDOG_AGGREGATE_STATE: WatchdogState id 0x02 update on change
    telemetry WATCHDOG_RECOVERY_LEVEL: WatchdogRecoveryLevel id 0x03 update on change
    telemetry WATCHDOG_FEED_ELIGIBLE: bool id 0x04 update on change
    telemetry WATCHDOG_WARNING_MASK: U32 id 0x05 update on change
    telemetry WATCHDOG_FAULT_MASK: U32 id 0x06 update on change
    telemetry WATCHDOG_SUPPRESS_MASK: U32 id 0x07 update on change
    telemetry WATCHDOG_SAFE_REQUEST_TOTAL: U32 id 0x08
    telemetry WATCHDOG_FEED_SUPPRESS_TOTAL: U32 id 0x09
    telemetry WATCHDOG_FEED_STROKE_ATTEMPT_TOTAL: U32 id 0x0A
    telemetry WATCHDOG_AGGREGATE_RECOVERY_TOTAL: U32 id 0x0B
    telemetry WATCHDOG_EPS_BRIDGE_STATE: WatchdogState id 0x0C update on change
    telemetry WATCHDOG_EPS_BRIDGE_AGE_TICKS: U32 id 0x0D
    telemetry WATCHDOG_EPS_FDIR_STATE: WatchdogState id 0x0E update on change
    telemetry WATCHDOG_EPS_FDIR_AGE_TICKS: U32 id 0x0F
    telemetry WATCHDOG_MODE_SAFETY_STATE: WatchdogState id 0x10 update on change
    telemetry WATCHDOG_MODE_SAFETY_AGE_TICKS: U32 id 0x11
    telemetry WATCHDOG_COMM_CONTROLLER_STATE: WatchdogState id 0x12 update on change
    telemetry WATCHDOG_COMM_CONTROLLER_AGE_TICKS: U32 id 0x13
    telemetry WATCHDOG_ADCS_FDIR_STATE: WatchdogState id 0x14 update on change
    telemetry WATCHDOG_ADCS_FDIR_AGE_TICKS: U32 id 0x15

  }

}
