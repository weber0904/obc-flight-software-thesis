module OBC {

  passive component UartDriver {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    event UART_OPEN \
      severity activity high \
      id 0x00 \
      format "UART link opened"

    event UART_ERROR(code: U32) \
      severity warning low \
      id 0x01 \
      format "UART link error {}"

    telemetry UART_TX_BYTES: U32 id 0x00
    telemetry UART_RX_BYTES: U32 id 0x01
    telemetry UART_TX_ERRORS: U32 id 0x02
    telemetry UART_RX_ERRORS: U32 id 0x03
    telemetry UART_CONNECTED: bool id 0x04 update on change

  }

}
