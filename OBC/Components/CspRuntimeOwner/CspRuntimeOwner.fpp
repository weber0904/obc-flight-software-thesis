module OBC {

  passive component CspRuntimeOwner {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    event CSP_OWNER_TIMEOUT(targetNode: U16, targetPort: U8, timeoutMs: U32) \
      severity warning high \
      id 0x00 \
      format "CSP owner timeout node {} port {} timeout {} ms"

    telemetry CSP_OWNER_QUEUE_DEPTH: U32 id 0x00
    telemetry CSP_OWNER_INFLIGHT: U32 id 0x01 update on change
    telemetry CSP_OWNER_TOTAL_TIMEOUTS: U32 id 0x02
    telemetry CSP_OWNER_TOTAL_COALESCED: U32 id 0x03
    telemetry CSP_OWNER_LAST_LATENCY_USEC: U32 id 0x04
    telemetry CSP_OWNER_LAST_RESULT: U32 id 0x05 update on change

  }

}
