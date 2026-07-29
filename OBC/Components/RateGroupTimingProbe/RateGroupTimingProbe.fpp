module OBC {

  passive component RateGroupTimingProbe {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: [16] Svc.Sched
    output port schedOut: [16] Svc.Sched

    event RG_TIMING_CONFIG(enabled: bool, cycleThresholdUsec: U32) \
      severity activity high \
      id 0x00 \
      format "Rate group timing probe enabled {} cycleThreshold {} us"

    event RG_TIMING_CYCLE_THRESHOLD_EXCEEDED(cycle: U32, totalUsec: U32, slowSlot: U32, slowUsec: U32) \
      severity warning low \
      id 0x01 \
      format "Rate group cycle {} total {} us slowSlot {} slowUsec {} us"

    telemetry RG_TIMING_LAST_SLOW_SLOT: U32 id 0x00 update on change
    telemetry RG_TIMING_LAST_SLOW_USEC: U32 id 0x01
    telemetry RG_TIMING_LAST_CYCLE_USEC: U32 id 0x02
    telemetry RG_TIMING_MAX_CYCLE_USEC: U32 id 0x03 update on change
    telemetry RG_TIMING_THRESHOLD_EXCEED_TOTAL: U32 id 0x04

  }

}
