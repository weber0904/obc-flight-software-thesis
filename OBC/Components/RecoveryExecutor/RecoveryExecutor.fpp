module OBC {

  passive component RecoveryExecutor {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    sync command GET_RECOVERY_STATUS() opcode 0x00

    event RECOVERY_INCIDENT_OPENED(source: RecoveryIncidentSource, level: RecoveryLevel, relatchCount: U32) \
      severity warning high \
      id 0x00 \
      format "Recovery incident {} opened at {} relatchCount {}"

    event RECOVERY_ACTION_REQUESTED(source: RecoveryIncidentSource, level: RecoveryLevel, recoveryAction: RecoveryAction) \
      severity warning high \
      id 0x01 \
      format "Recovery action requested source {} level {} action {}"

    event RECOVERY_ACTION_EXECUTED(source: RecoveryIncidentSource, recoveryAction: RecoveryAction, response: U32) \
      severity activity high \
      id 0x02 \
      format "Recovery action executed source {} action {} response {}"

    event RECOVERY_REBOOT_PENDING(source: RecoveryIncidentSource, level: RecoveryLevel) \
      severity warning high \
      id 0x03 \
      format "Recovery reboot pending source {} level {}"

    event RECOVERY_REBOOT_ISSUED(source: RecoveryIncidentSource, level: RecoveryLevel) \
      severity warning high \
      id 0x04 \
      format "Recovery reboot issued source {} level {}"

    event RECOVERY_INCIDENT_CLEARED(source: RecoveryIncidentSource, highestLevel: RecoveryLevel, relatchCount: U32) \
      severity activity high \
      id 0x05 \
      format "Recovery incident {} cleared after highest {} relatchCount {}"

    event RECOVERY_STATUS(activeIncidentCount: U32, activeSource: RecoveryIncidentSource, highestLevel: RecoveryLevel, lastAction: RecoveryAction, pendingProcessRestart: bool, pendingReboot: bool, relatchCount: U32) \
      severity activity high \
      id 0x06 \
      format "Recovery status activeCount {} source {} highest {} lastAction {} pendingProcessRestart {} pendingReboot {} relatchCount {}"

    telemetry RECOVERY_ACTIVE_INCIDENTS: U32 id 0x00 update on change
    telemetry RECOVERY_ACTIVE_SOURCE: RecoveryIncidentSource id 0x01 update on change
    telemetry RECOVERY_CURRENT_LEVEL: RecoveryLevel id 0x02 update on change
    telemetry RECOVERY_HIGHEST_LEVEL: RecoveryLevel id 0x03 update on change
    telemetry RECOVERY_LAST_ACTION: RecoveryAction id 0x04 update on change
    telemetry RECOVERY_PENDING_REBOOT: bool id 0x05 update on change
    telemetry RECOVERY_RELATCH_COUNT: U32 id 0x06 update on change
    telemetry RECOVERY_TOTAL_SAFE_FALLBACKS: U32 id 0x07
    telemetry RECOVERY_TOTAL_RESET_ACTIONS: U32 id 0x08
    telemetry RECOVERY_TOTAL_REBOOTS: U32 id 0x09
    telemetry RECOVERY_PENDING_PROCESS_RESTART: bool id 0x0A update on change
    telemetry RECOVERY_TOTAL_PROCESS_RESTARTS: U32 id 0x0B

  }

}
