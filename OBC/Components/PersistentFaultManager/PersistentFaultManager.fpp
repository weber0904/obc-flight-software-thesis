module OBC {

  passive component PersistentFaultManager {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    time get port Time

    sync command GET_PERSISTENT_FAULT_HISTORY(limit: U32) opcode 0x00

    event PERSISTENT_FAULT_HISTORY_STATUS(totalRecords: U32, returnedRecords: U32, activeCopy: PersistentFaultStoreCopy, generation: U32) \
      severity activity high \
      id 0x00 \
      format "Persistent fault history total {} returned {} activeCopy {} generation {}"

    event PERSISTENT_FAULT_HISTORY_RECORD(indexFromLatest: U32, kind: PersistentFaultRecordKind, source: RecoveryIncidentSource, level: RecoveryLevel, recoveryAction: RecoveryAction, resetCause: ResetCause, bootCount: U32, consecutiveResetCount: U32, uptimeSec: U32, timestampSec: U32, detail: U32, flags: U32) \
      severity activity high \
      id 0x01 \
      format "Persistent fault[{}] kind {} source {} level {} action {} cause {} boot {} consecutive {} uptime {} time {} detail {} flags {}"

    event PERSISTENT_FAULT_HISTORY_UNAVAILABLE() \
      severity warning high \
      id 0x02 \
      format "Persistent fault history unavailable"

    event PERSISTENT_FAULT_STORE_APPEND_FAILED(kind: PersistentFaultRecordKind) \
      severity warning high \
      id 0x03 \
      format "Persistent fault store append failed for {}"

  }

}
