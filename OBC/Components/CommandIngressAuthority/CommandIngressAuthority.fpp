module OBC {

  @ Keep two indexed paths so tests and later topologies can verify CmdDispatcher
  @ source-index preservation. Current hosted topologies wire index 0 only.
  constant CommandIngressAuthorityPorts = 2

  passive component CommandIngressAuthority {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus

    @ Incoming routed command packets before CmdDispatcher
    sync input port seqCmdBuffIn: [CommandIngressAuthorityPorts] Fw.Com

    @ Forwarded routed command packets after authority evaluation
    output port seqCmdBuffOut: [CommandIngressAuthorityPorts] Fw.Com

    @ Command status from CmdDispatcher
    sync input port seqCmdStatusIn: [CommandIngressAuthorityPorts] Fw.CmdResponse

    @ Command status or synthetic denial status to the upstream command-response sink
    output port seqCmdStatusOut: [CommandIngressAuthorityPorts] Fw.CmdResponse

    @ Secure link auth success notifications from SecureLinkAuthorizer
    sync input port authGrantedIn: SecureAuthGrantPort

    @ Secure link auth revoke notifications from SecureLinkAuthorizer
    sync input port authRevokedIn: SecureAuthRevocationPort

    @ Accepted secure-session activity refresh back into SecureLinkAuthorizer
    output port authActivityOut: SecureAuthActivityPort

    @ Governed official sequencing wrapper request path
    output port sequenceControlOut: SequenceControl

    @ Deferred completion for governed official sequencing wrapper commands
    sync input port sequenceControlStatusIn: SequenceControlStatus

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    event COMMAND_AUTHORITY_REJECTED(commandOpcode: U32, ingressPort: U32, linkIdentity: U32, linkRole: U32, commandClass: U32, reason: U32, response: Fw.CmdResponse) \
      severity warning high \
      id 0x00 \
      format "Command authority rejected opcode 0x{x} ingress {} identity {} role {} class {} reason {} response {}" \
      throttle 5

    event COMMAND_ENVELOPE_OBSERVED(ingressPort: U32, linkIdentity: U32, linkRole: U32, sessionId: U32, sequenceNumber: U32, innerOpcode: U32) \
      severity activity high \
      id 0x01 \
      format "Command envelope observed ingress {} identity {} role {} session {} sequence {} inner opcode 0x{x}" \
      throttle 5

    event COMMAND_ENVELOPE_REJECTED(ingressPort: U32, reason: U32, response: Fw.CmdResponse) \
      severity warning high \
      id 0x02 \
      format "Command envelope rejected ingress {} reason {} response {}" \
      throttle 5

    event COMMAND_ENVELOPE_AUTH_REJECTED(ingressPort: U32, linkIdentity: U32, linkRole: U32, sourceId: U32, keySlot: U32, sessionId: U32, sequenceNumber: U32, innerOpcode: U32, reason: U32, response: Fw.CmdResponse) \
      severity warning high \
      id 0x03 \
      format "Command envelope auth rejected ingress {} identity {} role {} source {} key slot {} session {} sequence {} inner opcode 0x{x} reason {} response {}" \
      throttle 5

    event COMMAND_SEQUENCE_REJECTED(ingressPort: U32, linkIdentity: U32, linkRole: U32, sessionId: U32, sequenceNumber: U32, innerOpcode: U32, reason: U32, response: Fw.CmdResponse) \
      severity warning high \
      id 0x04 \
      format "Command sequence rejected ingress {} identity {} role {} session {} sequence {} inner opcode 0x{x} reason {} response {}" \
      throttle 5

    event COMMAND_SESSION_OPENED(ingressPort: U32, linkIdentity: U32, linkRole: U32, sessionId: U32, replaced: U32) \
      severity activity high \
      id 0x05 \
      format "Command session opened ingress {} identity {} role {} session {} replaced {}" \
      throttle 5

    event COMMAND_SESSION_REJECTED(ingressPort: U32, linkIdentity: U32, linkRole: U32, sessionId: U32, sequenceNumber: U32, innerOpcode: U32, reason: U32, response: Fw.CmdResponse) \
      severity warning high \
      id 0x06 \
      format "Command session rejected ingress {} identity {} role {} session {} sequence {} inner opcode 0x{x} reason {} response {}" \
      throttle 5

    event COMMAND_SESSION_REVOKED(ingressPort: U32, linkIdentity: U32, linkRole: U32, sessionId: U32, reason: U32) \
      severity activity high \
      id 0x07 \
      format "Command session revoked ingress {} identity {} role {} session {} reason {}" \
      throttle 5

    event COMMAND_SESSION_FRESHNESS_STORE_FAULT(ingressPort: U32, linkIdentity: U32, linkRole: U32, sessionId: U32, operation: U32, status: U32) \
      severity warning high \
      id 0x08 \
      format "Command session freshness store fault ingress {} identity {} role {} session {} operation {} status {}" \
      throttle 5

    event SECURE_COMMAND_REJECTED(ingressPort: U32, linkIdentity: U32, linkRole: U32, sessionId: U32, sequenceNumber: U32, innerOpcode: U32, reason: U32, response: Fw.CmdResponse) \
      severity warning high \
      id 0x09 \
      format "Secure command rejected ingress {} identity {} role {} session {} sequence {} inner opcode 0x{x} reason {} response {}" \
      throttle 5

    telemetry AUTH_REJECT_TOTAL: U32 id 0x00 update on change
    telemetry AUTH_REJECT_POLICY: U32 id 0x01 update on change
    telemetry AUTH_REJECT_MALFORMED: U32 id 0x02 update on change
    telemetry AUTH_REJECT_UNKNOWN_OPCODE: U32 id 0x03 update on change
    telemetry AUTH_REJECT_CONFIG: U32 id 0x04 update on change
    telemetry AUTH_REJECT_UHF_BACKUP: U32 id 0x05 update on change
    telemetry AUTH_LAST_REJECT_OPCODE: U32 id 0x06 update on change
    telemetry AUTH_LAST_REJECT_REASON: U32 id 0x07 update on change
    telemetry AUTH_LAST_REJECT_CLASS: U32 id 0x08 update on change
    telemetry AUTH_LAST_REJECT_ROLE: U32 id 0x09 update on change
    telemetry AUTH_LAST_REJECT_PORT: U32 id 0x0A update on change
    telemetry AUTH_LAST_REJECT_IDENTITY: U32 id 0x0B update on change
    telemetry ENVELOPE_OBSERVED_TOTAL: U32 id 0x0C update on change
    telemetry ENVELOPE_REJECTED_TOTAL: U32 id 0x0D update on change
    telemetry ENVELOPE_LAST_SESSION_ID: U32 id 0x0E update on change
    telemetry ENVELOPE_LAST_SEQUENCE_NUMBER: U32 id 0x0F update on change
    telemetry ENVELOPE_LAST_INNER_OPCODE: U32 id 0x10 update on change
    telemetry ENVELOPE_LAST_REJECT_REASON: U32 id 0x11 update on change
    telemetry ENVELOPE_AUTH_REJECT_TOTAL: U32 id 0x12 update on change
    telemetry ENVELOPE_AUTH_REJECT_CONFIG: U32 id 0x13 update on change
    telemetry ENVELOPE_AUTH_REJECT_SOURCE_MISMATCH: U32 id 0x14 update on change
    telemetry ENVELOPE_AUTH_REJECT_UNKNOWN_KEY_SLOT: U32 id 0x15 update on change
    telemetry ENVELOPE_AUTH_REJECT_BAD_MAC: U32 id 0x16 update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_PORT: U32 id 0x17 update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_IDENTITY: U32 id 0x18 update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_ROLE: U32 id 0x19 update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_SOURCE_ID: U32 id 0x1A update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_KEY_SLOT: U32 id 0x1B update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_SESSION_ID: U32 id 0x1C update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_SEQUENCE_NUMBER: U32 id 0x1D update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_INNER_OPCODE: U32 id 0x1E update on change
    telemetry ENVELOPE_AUTH_LAST_REJECT_REASON: U32 id 0x1F update on change
    telemetry SEQUENCE_REJECT_TOTAL: U32 id 0x20 update on change
    telemetry SEQUENCE_REJECT_NOT_INCREASING: U32 id 0x21 update on change
    telemetry SEQUENCE_REJECT_WINDOW_FULL: U32 id 0x22 update on change
    telemetry SEQUENCE_LAST_REJECT_PORT: U32 id 0x23 update on change
    telemetry SEQUENCE_LAST_REJECT_IDENTITY: U32 id 0x24 update on change
    telemetry SEQUENCE_LAST_REJECT_ROLE: U32 id 0x25 update on change
    telemetry SEQUENCE_LAST_REJECT_SESSION_ID: U32 id 0x26 update on change
    telemetry SEQUENCE_LAST_REJECT_SEQUENCE_NUMBER: U32 id 0x27 update on change
    telemetry SEQUENCE_LAST_REJECT_INNER_OPCODE: U32 id 0x28 update on change
    telemetry SEQUENCE_LAST_REJECT_REASON: U32 id 0x29 update on change
    telemetry SESSION_OPEN_TOTAL: U32 id 0x2A update on change
    telemetry SESSION_REJECT_TOTAL: U32 id 0x2B update on change
    telemetry SESSION_ACTIVE: U32 id 0x2C update on change
    telemetry SESSION_ACTIVE_PORT: U32 id 0x2D update on change
    telemetry SESSION_ACTIVE_IDENTITY: U32 id 0x2E update on change
    telemetry SESSION_ACTIVE_ROLE: U32 id 0x2F update on change
    telemetry SESSION_ACTIVE_ID: U32 id 0x30 update on change
    telemetry SESSION_LAST_ACCEPTED_SEQUENCE: U32 id 0x31 update on change
    telemetry SESSION_LAST_REJECT_PORT: U32 id 0x32 update on change
    telemetry SESSION_LAST_REJECT_IDENTITY: U32 id 0x33 update on change
    telemetry SESSION_LAST_REJECT_ROLE: U32 id 0x34 update on change
    telemetry SESSION_LAST_REJECT_SESSION_ID: U32 id 0x35 update on change
    telemetry SESSION_LAST_REJECT_SEQUENCE_NUMBER: U32 id 0x36 update on change
    telemetry SESSION_LAST_REJECT_INNER_OPCODE: U32 id 0x37 update on change
    telemetry SESSION_LAST_REJECT_REASON: U32 id 0x38 update on change
    telemetry SESSION_REVOKE_TOTAL: U32 id 0x39 update on change
    telemetry SESSION_LAST_REVOKE_PORT: U32 id 0x3A update on change
    telemetry SESSION_LAST_REVOKE_IDENTITY: U32 id 0x3B update on change
    telemetry SESSION_LAST_REVOKE_ROLE: U32 id 0x3C update on change
    telemetry SESSION_LAST_REVOKE_ID: U32 id 0x3D update on change
    telemetry SESSION_LAST_REVOKE_REASON: U32 id 0x3E update on change
    telemetry SESSION_PERSISTENCE_AVAILABLE: U32 id 0x3F update on change
    telemetry SESSION_PERSISTENCE_ACTIVE_COPY: U32 id 0x40 update on change
    telemetry SESSION_PERSISTENCE_GENERATION: U32 id 0x41 update on change
    telemetry SESSION_PERSISTENCE_LOAD_FAULT_TOTAL: U32 id 0x42 update on change
    telemetry SESSION_PERSISTENCE_SAVE_FAULT_TOTAL: U32 id 0x43 update on change
    telemetry SESSION_PERSISTED_FLOOR_PORT: U32 id 0x44 update on change
    telemetry SESSION_PERSISTED_FLOOR_IDENTITY: U32 id 0x45 update on change
    telemetry SESSION_PERSISTED_FLOOR_ROLE: U32 id 0x46 update on change
    telemetry SESSION_PERSISTED_FLOOR: U32 id 0x47 update on change
    telemetry SECURE_COMMAND_REJECT_TOTAL: U32 id 0x48 update on change
    telemetry SECURE_COMMAND_REJECT_BAD_MAC: U32 id 0x49 update on change
    telemetry SECURE_COMMAND_REJECT_NO_AUTH: U32 id 0x4A update on change
    telemetry SECURE_COMMAND_LAST_REJECT_PORT: U32 id 0x4B update on change
    telemetry SECURE_COMMAND_LAST_REJECT_IDENTITY: U32 id 0x4C update on change
    telemetry SECURE_COMMAND_LAST_REJECT_ROLE: U32 id 0x4D update on change
    telemetry SECURE_COMMAND_LAST_REJECT_SESSION_ID: U32 id 0x4E update on change
    telemetry SECURE_COMMAND_LAST_REJECT_SEQUENCE_NUMBER: U32 id 0x4F update on change
    telemetry SECURE_COMMAND_LAST_REJECT_INNER_OPCODE: U32 id 0x50 update on change
    telemetry SECURE_COMMAND_LAST_REJECT_REASON: U32 id 0x51 update on change
    telemetry SECURE_SESSION_ACTIVE_SERVICE: U32 id 0x52 update on change

  }

}
