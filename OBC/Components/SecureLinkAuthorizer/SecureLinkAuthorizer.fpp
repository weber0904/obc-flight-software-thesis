module OBC {

  constant SecureLinkAuthorizerPorts = 2

  passive component SecureLinkAuthorizer {

    sync input port handshakeUplinkIn: [SecureLinkAuthorizerPorts] Svc.ComDataWithContext
    output port bufferReturnOut: [SecureLinkAuthorizerPorts] Fw.BufferSend
    output port handshakePacketOut: [SecureLinkAuthorizerPorts] Fw.Com

    output port authGrantedOut: [2] SecureAuthGrantPort
    output port authRevokedOut: [2] SecureAuthRevocationPort

    sync input port authActivityIn: [2] SecureAuthActivityPort
    sync input port authInvalidateIn: SecureAuthRevocationPort
    sync input port schedIn: Svc.Sched

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    event SECURE_AUTH_PACKET_REJECTED(ingressPort: U32, serviceId: U32, reason: U32) \
      severity warning high \
      id 0x00 \
      format "Secure auth packet rejected ingress {} service {} reason {}" \
      throttle 10

    event SECURE_AUTH_CHALLENGE_ISSUED(ingressPort: U32, serviceId: U32) \
      severity activity high \
      id 0x01 \
      format "Secure auth challenge issued ingress {} service {}" \
      throttle 10

    event SECURE_AUTH_ESTABLISHED(ingressPort: U32, serviceId: U32) \
      severity activity high \
      id 0x02 \
      format "Secure auth established ingress {} service {}" \
      throttle 10

    event SECURE_AUTH_REVOKED(ingressPort: U32, serviceId: U32, reason: U32) \
      severity activity high \
      id 0x03 \
      format "Secure auth revoked ingress {} service {} reason {}" \
      throttle 10

    telemetry SECURE_AUTH_ACTIVE: U32 id 0x00 update on change
    telemetry SECURE_AUTH_ACTIVE_PORT: U32 id 0x01 update on change
    telemetry SECURE_AUTH_ACTIVE_SERVICE: U32 id 0x02 update on change
    telemetry SECURE_AUTH_LAST_STATUS: U32 id 0x03 update on change
    telemetry SECURE_AUTH_LAST_REJECT_REASON: U32 id 0x04 update on change
    telemetry SECURE_AUTH_CHALLENGE_TOTAL: U32 id 0x05 update on change
    telemetry SECURE_AUTH_ESTABLISHED_TOTAL: U32 id 0x06 update on change
    telemetry SECURE_AUTH_REVOKE_TOTAL: U32 id 0x07 update on change
    telemetry SECURE_AUTH_TIMEOUT_REMAINING: U32 id 0x08 update on change

  }

}
