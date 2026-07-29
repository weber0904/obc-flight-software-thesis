module OBC {

  passive component BeaconPublisher {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    event BEACON_PACKET_EMITTED(sequence: U32, sizeBytes: U32) \
      severity activity low \
      id 0x00 \
      format "Beacon packet emitted sequence {} size {}"

    event BEACON_SOURCE_UNAVAILABLE \
      severity warning high \
      id 0x01 \
      format "Beacon reduced-state source unavailable"

    event BEACON_SINK_ERROR(sequence: U32) \
      severity warning high \
      id 0x02 \
      format "Beacon sink rejected sequence {}"

    event BEACON_NOT_CONFIGURED \
      severity warning high \
      id 0x03 \
      format "Beacon publisher not configured"

    telemetry BEACON_SEQUENCE: U32 id 0x00
    telemetry BEACON_EMITTED_COUNT: U32 id 0x01
    telemetry BEACON_LAST_SIZE_BYTES: U32 id 0x02
    telemetry BEACON_LAST_ERROR: U32 id 0x03 update on change

  }

}
