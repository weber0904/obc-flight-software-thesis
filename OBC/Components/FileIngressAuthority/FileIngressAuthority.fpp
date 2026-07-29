module OBC {

  passive component FileIngressAuthority {

    guarded input port bufferSendIn: [2] Fw.BufferSend
    guarded input port bufferReturnIn: Fw.BufferSend
    sync input port authGrantedIn: SecureAuthGrantPort
    sync input port authRevokedIn: SecureAuthRevocationPort
    sync input port filePolicyIn: FileIngressPolicyPort

    output port bufferSendOut: Fw.BufferSend
    output port bufferReturnOut: [2] Fw.BufferSend
    output port authActivityOut: SecureAuthActivityPort

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    event FILE_INGRESS_START_ACCEPTED(path: string size 255) \
      severity activity low \
      id 0x00 \
      format "File ingress accepted start path {}" \
      throttle 5

    event FILE_INGRESS_START_REJECTED(path: string size 255, reason: U32) \
      severity warning high \
      id 0x01 \
      format "File ingress rejected start path {} reason {}" \
      throttle 5

    event FILE_INGRESS_PACKET_DROPPED(packetType: U32, reason: U32) \
      severity warning low \
      id 0x02 \
      format "File ingress dropped packet type {} reason {}" \
      throttle 5

    telemetry FILE_INGRESS_ACCEPTED_STARTS: U32 id 0x00 update on change
    telemetry FILE_INGRESS_REJECTED_STARTS: U32 id 0x01 update on change
    telemetry FILE_INGRESS_DROPPED_PACKETS: U32 id 0x02 update on change
    telemetry FILE_INGRESS_DROP_MODE: U32 id 0x03 update on change
    telemetry FILE_INGRESS_AUTH_ACTIVE: U32 id 0x04 update on change
    telemetry FILE_INGRESS_ALLOWED_PORTS: U32 id 0x05 update on change

  }

}
