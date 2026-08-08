module OBC {

  passive component BootManager {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    output port bootStatusRefreshTlmOut: Fw.Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    sync command BOOT_STATUS() opcode 0x00
    sync command BOOT_PREPARE_UPDATE(imageSize: U32, digest: string size 64) opcode 0x01
    sync command BOOT_VERIFY_STAGED_IMAGE(stagingPath: string size 128) opcode 0x02
    sync command BOOT_ACTIVATE_STAGED_IMAGE() opcode 0x03
    sync command BOOT_CONFIRM() opcode 0x04
    sync command BOOT_ROLLBACK() opcode 0x05
    sync command GET_RESET_CAUSE() opcode 0x06
    sync command GET_BOOT_COUNT() opcode 0x07

    event BOOT_UPDATE_PREPARED(imageSize: U32) \
      severity activity high \
      id 0x00 \
      format "Boot update prepared for {} bytes"

    event BOOT_STAGE_VERIFY_OK(targetSlot: BootSlot) \
      severity activity high \
      id 0x01 \
      format "Boot staging image verified for {}"

    event BOOT_STAGE_VERIFY_FAIL(code: U32) \
      severity warning high \
      id 0x02 \
      format "Boot staging image verification failed code {}"

    event BOOT_SLOT_SWITCHED(activeSlot: BootSlot, pendingSlot: BootSlot) \
      severity activity high \
      id 0x03 \
      format "Boot active slot {} pending slot {}"

    event BOOT_VERSION_CONFIRMED(activeSlot: BootSlot) \
      severity activity high \
      id 0x04 \
      format "Boot version confirmed on {}"

    event BOOT_ROLLBACK_TRIGGERED(activeSlot: BootSlot, reason: U32) \
      severity warning high \
      id 0x05 \
      format "Boot rollback switched to {} reason {}"

    event BOOT_TRUST_ACCEPTED(targetSlot: BootSlot, softwareVersion: U32, keySlot: U32) \
      severity activity high \
      id 0x06 \
      format "Boot trust accepted for {} version {} key slot {}"

    event BOOT_TRUST_REJECTED(reason: U32, softwareVersion: U32, keySlot: U32) \
      severity warning high \
      id 0x07 \
      format "Boot trust rejected reason {} version {} key slot {}"

    event BOOT_RECOVERY_INTENT_RECORDED(resetCause: ResetCause, source: RecoveryIncidentSource, level: RecoveryLevel) \
      severity warning high \
      id 0x08 \
      format "Boot recovery intent recorded cause {} source {} level {}"

    event BOOT_RECOVERY_STATUS(resetCause: ResetCause, bootCount: U32, consecutiveResetCount: U32, safeFallbackRequired: bool, source: RecoveryIncidentSource, level: RecoveryLevel) \
      severity activity high \
      id 0x09 \
      format "Boot recovery status cause {} bootCount {} consecutive {} safeFallback {} source {} level {}"

    event BOOT_RECOVERY_STABLE_ACK(resetCause: ResetCause, bootCount: U32) \
      severity activity high \
      id 0x0A \
      format "Boot recovery stable ack cause {} bootCount {}"

    telemetry BOOT_ACTIVE_SLOT: BootSlot id 0x00 update on change
    telemetry BOOT_PENDING_SLOT: BootSlot id 0x01 update on change
    telemetry BOOT_CONFIRMED: bool id 0x02 update on change
    telemetry BOOT_UPDATE_PROGRESS: U8 id 0x03 update on change
    telemetry BOOT_LAST_ERROR: U32 id 0x04 update on change
    telemetry BOOT_TRUST_STATUS: U32 id 0x05 update on change
    telemetry BOOT_TRUST_REJECT_REASON: U32 id 0x06 update on change
    telemetry BOOT_STAGED_VERSION: U32 id 0x07 update on change
    telemetry BOOT_LAST_ACCEPTED_VERSION: U32 id 0x08 update on change
    telemetry BOOT_RESET_CAUSE: ResetCause id 0x09 update on change
    telemetry BOOT_BOOT_COUNT: U32 id 0x0A update on change
    telemetry BOOT_CONSECUTIVE_RESET_COUNT: U32 id 0x0B update on change
    telemetry BOOT_SAFE_FALLBACK_REQUIRED: bool id 0x0C update on change
    telemetry BOOT_LAST_RECOVERY_SOURCE: RecoveryIncidentSource id 0x0D update on change
    telemetry BOOT_LAST_RECOVERY_LEVEL: RecoveryLevel id 0x0E update on change

  }

}
