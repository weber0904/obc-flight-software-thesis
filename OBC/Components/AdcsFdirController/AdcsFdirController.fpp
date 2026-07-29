module OBC {

  passive component AdcsFdirController {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched
    output port watchdogBeatOut: Svc.WatchDog

    event ADCS_FDIR_RETRYING(source: RecoveryIncidentSource, failureCount: U32) \
      severity warning low \
      id 0x00 \
      format "ADCS FDIR retrying source {} failureCount {}"

    event ADCS_FDIR_FAULT_ENTERED(source: RecoveryIncidentSource, failureCount: U32) \
      severity warning high \
      id 0x01 \
      format "ADCS FDIR fault entered source {} failureCount {}"

    event ADCS_FDIR_FAULT_CLEARED(source: RecoveryIncidentSource, failureCount: U32) \
      severity activity high \
      id 0x02 \
      format "ADCS FDIR fault cleared source {} lastFailureCount {}"

    telemetry ADCS_FDIR_FAULT_LATCHED: bool id 0x00 update on change
    telemetry ADCS_FDIR_ACTIVE_SOURCE: RecoveryIncidentSource id 0x01 update on change
    telemetry ADCS_FDIR_LAST_FAILURE_COUNT: U32 id 0x02 update on change
    telemetry ADCS_FDIR_ESCALATION_COUNT: U32 id 0x03
    telemetry ADCS_FDIR_RECOVERY_COUNT: U32 id 0x04

  }

}
