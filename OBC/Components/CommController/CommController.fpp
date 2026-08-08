module OBC {

  passive component CommController {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched
    guarded input port dpFileRequestIn: Svc.SendFileRequest
    sync input port fileCompleteIn: Svc.SendFileComplete

    output port sendFileOut: Svc.SendFileRequest
    output port dpFileCompleteOut: Svc.SendFileComplete
    output port watchdogBeatOut: Svc.WatchDog
    output port secureAuthInvalidateOut: SecureAuthRevocationPort
    output port filePolicyOut: FileIngressPolicyPort
    output port commStatusRefreshTlmOut: Fw.Tlm

    sync command COMM_SET_ACTIVE(band: CommBand) opcode 0x00
    sync command COMM_START_PASS(durationSec: U32) opcode 0x01
    sync command COMM_STOP_PASS() opcode 0x02
    sync command COMM_GET_STATUS() opcode 0x03

    event COMM_BAND_SWITCH(band: CommBand) \
      severity activity high \
      id 0x00 \
      format "Comm active band switched to {}"

    event COMM_PASS_START(durationSec: U32) \
      severity activity high \
      id 0x01 \
      format "Comm pass started for {} sec"

    event COMM_PASS_END(totalPasses: U32) \
      severity activity high \
      id 0x02 \
      format "Comm pass ended, total passes {}"

    event COMM_PRIMARY_LINK_CHANGED(primaryCommand: CommBand, primaryTelemetry: CommBand, primaryFile: CommBand, reason: U32) \
      severity activity high \
      id 0x03 \
      format "Comm primary links command {} telemetry {} file {} reason {}"

    event COMM_LINK_AVAILABILITY_CHANGED(band: CommBand, available: bool) \
      severity activity low \
      id 0x04 \
      format "Comm link {} available {}"

    event COMM_DOWNLINK_STATE_CHANGED(activeOwner: U32, pendingOwner: U32, reason: U32) \
      severity activity high \
      id 0x05 \
      format "Comm downlink active owner {} pending owner {} reason {}"

    event COMM_RECOVERY_FAILOVER_RESULT(switched: bool, noHealthyBackup: bool, sessionsRevoked: U32, ownersCleared: U32, finalPrimaryCommand: CommBand, finalPrimaryTelemetry: CommBand, finalPrimaryFile: CommBand) \
      severity activity high \
      id 0x06 \
      format "Comm recovery failover switched {} noHealthyBackup {} sessionsRevoked {} ownersCleared {} final command {} telemetry {} file {}"

    event COMM_RT_TRANSFER_STARTED(transferId: U32, fileSize: U32, totalSegments: U32) \
      severity activity high \
      id 0x07 \
      format "Comm reliable transfer {} started bytes {} segments {}"

    event COMM_RT_PROGRESS(transferId: U32, contiguousSegments: U32, totalSegments: U32, committedBytes: U32, duplicateSegments: U32) \
      severity activity low \
      id 0x08 \
      format "Comm reliable transfer {} progress segments {}/{} bytes {} duplicates {}"

    event COMM_RT_RESEND(transferId: U32, resendCount: U32, contiguousSegments: U32) \
      severity activity high \
      id 0x09 \
      format "Comm reliable transfer {} resend {} contiguousSegments {}"

    event COMM_RT_RETRY_EXHAUSTED(transferId: U32, resendCount: U32, contiguousSegments: U32, committedBytes: U32) \
      severity warning high \
      id 0x0A \
      format "Comm reliable transfer {} retry exhausted resendCount {} contiguousSegments {} bytes {}"

    event COMM_RT_FINAL_RESULT(transferId: U32, result: U32, committedBytes: U32, duplicateSegments: U32) \
      severity activity high \
      id 0x0B \
      format "Comm reliable transfer {} final result {} bytes {} duplicates {}"

    event COMM_RT_ROUTE_SELECTED(requestContext: U32, owner: U32) \
      severity activity high \
      id 0x0C \
      format "Comm reliable transfer route selected context {} owner {}"

    event COMM_RT_START_FAILED(requestContext: U32, stage: U32, detail: U32) \
      severity warning high \
      id 0x0D \
      format "Comm reliable transfer start failed context {} stage {} detail {}"

    event COMM_UHF_BEACON_SUPPRESS_STARTED(ingressPort: U32, linkRole: U32, sessionId: U32, timeoutTicks: U32, replaced: bool) \
      severity activity high \
      id 0x0E \
      format "Comm UHF beacon suppress started ingress {} role {} session {} timeoutTicks {} replaced {}"

    event COMM_UHF_BEACON_SUPPRESS_REFRESHED(ingressPort: U32, linkRole: U32, sessionId: U32, sequenceNumber: U32, remainingTicks: U32) \
      severity activity low \
      id 0x0F \
      format "Comm UHF beacon suppress refreshed ingress {} role {} session {} sequence {} remainingTicks {}"

    event COMM_UHF_BEACON_SUPPRESS_CLEARED(reason: U32, ingressPort: U32, linkRole: U32, sessionId: U32, lastSequence: U32) \
      severity activity high \
      id 0x10 \
      format "Comm UHF beacon suppress cleared reason {} ingress {} role {} session {} lastSequence {}"

    event COMM_S_BAND_LIVE_OBSERVABILITY_CHANGED(enabled: U32, reason: U32, ingressPort: U32, linkRole: U32, sessionId: U32) \
      severity activity high \
      id 0x11 \
      format "Comm S-band live observability active {} reason {} ingress {} role {} session {}"

    telemetry COMM_ACTIVE_BAND: CommBand id 0x00 update on change
    telemetry COMM_PASS_ACTIVE: bool id 0x01 update on change
    telemetry COMM_PASS_REMAINING: U32 id 0x02
    telemetry COMM_TOTAL_PASSES: U32 id 0x03
    telemetry COMM_PRIMARY_COMMAND_LINK: CommBand id 0x04 update on change
    telemetry COMM_PRIMARY_TELEMETRY_LINK: CommBand id 0x05 update on change
    telemetry COMM_PRIMARY_FILE_LINK: CommBand id 0x06 update on change
    telemetry COMM_S_BAND_AVAILABLE: bool id 0x07 update on change
    telemetry COMM_UHF_AVAILABLE: bool id 0x08 update on change
    telemetry COMM_DOWNLINK_ACTIVE_OWNER: U32 id 0x09 update on change
    telemetry COMM_DOWNLINK_PENDING_OWNER: U32 id 0x0A update on change
    telemetry COMM_SESSION_REVOKE_TOTAL: U32 id 0x0B update on change
    telemetry COMM_DOWNLINK_REJECT_TOTAL: U32 id 0x0C update on change
    telemetry COMM_FDIR_FAULT_LATCHED: bool id 0x0D update on change
    telemetry COMM_FDIR_FAULT_KIND: U32 id 0x0E update on change
    telemetry COMM_FDIR_CONSEC_PRIMARY_UNAVAILABLE: U32 id 0x0F update on change
    telemetry COMM_FDIR_CONSEC_PRIMARY_TRANSPORT: U32 id 0x10 update on change
    telemetry COMM_RECOVERY_FAILOVER_TOTAL: U32 id 0x11
    telemetry COMM_RECOVERY_OWNER_CLEAR_TOTAL: U32 id 0x12
    telemetry COMM_S_BAND_ACTIVITY_AGE_TICKS: U32 id 0x13
    telemetry COMM_UHF_ACTIVITY_AGE_TICKS: U32 id 0x14
    telemetry COMM_S_BAND_AVAILABILITY_REASON: U32 id 0x15 update on change
    telemetry COMM_UHF_AVAILABILITY_REASON: U32 id 0x16 update on change
    telemetry COMM_RT_ACTIVE_TRANSFER_ID: U32 id 0x17 update on change
    telemetry COMM_RT_LAST_ACK_SEGMENT: U32 id 0x18
    telemetry COMM_RT_ACKED_BYTES: U32 id 0x19
    telemetry COMM_RT_RESEND_TOTAL: U32 id 0x1A
    telemetry COMM_RT_LAST_RESULT: U32 id 0x1B update on change
    telemetry COMM_UHF_BEACON_SUPPRESS_ACTIVE: bool id 0x1C update on change
    telemetry COMM_UHF_BEACON_SUPPRESS_INGRESS_PORT: U32 id 0x1D update on change
    telemetry COMM_UHF_BEACON_SUPPRESS_ROLE: U32 id 0x1E update on change
    telemetry COMM_UHF_BEACON_SUPPRESS_SESSION_ID: U32 id 0x1F update on change
    telemetry COMM_UHF_BEACON_SUPPRESS_LAST_SEQUENCE: U32 id 0x20
    telemetry COMM_UHF_BEACON_SUPPRESS_REMAINING_TICKS: U32 id 0x21
    telemetry COMM_UHF_BEACON_SUPPRESS_TIMEOUT_TICKS: U32 id 0x22 update on change
    telemetry COMM_S_BAND_LIVE_OBSERVABILITY_ACTIVE: bool id 0x23 update on change
    telemetry COMM_S_BAND_LIVE_OBSERVABILITY_REASON: U32 id 0x24 update on change
    telemetry COMM_S_BAND_LIVE_OBSERVABILITY_INGRESS_PORT: U32 id 0x25 update on change
    telemetry COMM_S_BAND_LIVE_OBSERVABILITY_ROLE: U32 id 0x26 update on change
    telemetry COMM_S_BAND_LIVE_OBSERVABILITY_SESSION_ID: U32 id 0x27 update on change
    telemetry COMM_S_BAND_LIVE_OBSERVABILITY_LAST_SEQUENCE: U32 id 0x28

  }

}
