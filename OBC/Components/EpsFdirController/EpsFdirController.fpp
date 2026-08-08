module OBC {

  passive component EpsFdirController {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched
    output port watchdogBeatOut: Svc.WatchDog

    event EPS_FDIR_RETRYING(failureCount: U32) \
      severity warning low \
      id 0x00 \
      format "EPS FDIR retrying after consecutive failure count {}"

    event EPS_FDIR_FAULT_ENTERED(failureCount: U32, currentMode: SatMode, requestedSafe: bool) \
      severity warning high \
      id 0x01 \
      format "EPS FDIR fault latched at failure count {} mode {} requestedSafe {}"

    event EPS_FDIR_FAULT_CLEARED(failureCount: U32) \
      severity activity high \
      id 0x02 \
      format "EPS FDIR fault cleared after recovery from failure count {}"

    telemetry EPS_FDIR_FAULT_LATCHED: bool id 0x00 update on change
    telemetry EPS_FDIR_LAST_FAILURE_COUNT: U32 id 0x01
    telemetry EPS_FDIR_ESCALATION_COUNT: U32 id 0x02
    telemetry EPS_FDIR_RECOVERY_COUNT: U32 id 0x03
    telemetry EPS_FDIR_LAST_REQUESTED_SAFE: bool id 0x04

  }

}
