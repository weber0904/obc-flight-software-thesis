module OBC {

  passive component GroundLinkDriver {

    import Drv.ByteStreamDriver

    @ Allocation for received data
    output port allocate: Fw.BufferGet

    @ Deallocation of allocated buffers
    output port deallocate: Fw.BufferSend

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    event GROUND_LINK_UP(mode: U8) \
      severity activity high \
      id 0x00 \
      format "Ground link connected mode {}"

    event GROUND_LINK_DOWN(mode: U8) \
      severity warning low \
      id 0x01 \
      format "Ground link disconnected mode {}"

    event GROUND_LINK_ERROR(code: U32) \
      severity warning low \
      id 0x02 \
      format "Ground link error {}"

    telemetry GROUND_LINK_MODE: U8 id 0x00 update on change
    telemetry GROUND_LINK_CONNECTED: bool id 0x01 update on change
    telemetry GROUND_LINK_TX_CHUNKS: U32 id 0x02
    telemetry GROUND_LINK_RX_CHUNKS: U32 id 0x03
    telemetry GROUND_LINK_TX_BYTES: U32 id 0x04
    telemetry GROUND_LINK_RX_BYTES: U32 id 0x05
    telemetry GROUND_LINK_TX_ERRORS: U32 id 0x06
    telemetry GROUND_LINK_RX_ERRORS: U32 id 0x07

  }

}
