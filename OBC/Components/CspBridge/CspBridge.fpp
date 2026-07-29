module OBC {

  passive component CspBridge {

    command recv port CmdDisp
    command reg port CmdReg
    command resp port CmdStatus
    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync command CSP_INIT(nodeId: U8) opcode 0x00
    sync command CSP_PING(targetNode: U8, timeoutMs: U32) opcode 0x01
    sync command CSP_SEND_RAW(targetNode: U8, targetPort: U8, data: string size 240) opcode 0x02

    event CSP_INIT_COMPLETE(nodeId: U8) \
      severity activity high \
      id 0x00 \
      format "CSP initialized for node {}"

    event CSP_PING_RESULT(targetNode: U8, success: bool, timeoutMs: U32) \
      severity activity low \
      id 0x01 \
      format "CSP ping node {} success {} timeout {} ms"

    event CSP_PING_SUCCESS_LOCAL(targetNode: U8, timeoutMs: U32) \
      severity diagnostic \
      id 0x04 \
      format "CSP ping node {} success 1 timeout {} ms"

    event CSP_RAW_SENT(targetNode: U8, targetPort: U8, bytes: U32) \
      severity activity low \
      id 0x02 \
      format "CSP raw packet sent to {}:{} ({} bytes)"

    event CSP_ERROR(targetNode: U8, code: U32) \
      severity warning high \
      id 0x03 \
      format "CSP operation error for node {} code {}"

    telemetry CSP_TX_PACKETS: U32 id 0x00 update on change
    telemetry CSP_RX_PACKETS: U32 id 0x01 update on change
    telemetry CSP_ERROR_COUNT: U32 id 0x02 update on change
    telemetry CSP_FREE_BUFFERS: U32 id 0x03 update on change

  }

}
