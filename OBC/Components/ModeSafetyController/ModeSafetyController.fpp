module OBC {

  passive component ModeSafetyController {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched
    output port watchdogBeatOut: Svc.WatchDog

    event MODE_SAFETY_EPS_UNAVAILABLE \
      severity warning high \
      id 0x00 \
      format "Mode safety EPS status unavailable"

    event MODE_SAFETY_TRANSITION(fromMode: SatMode, toMode: SatMode, soc: F32) \
      severity activity high \
      id 0x01 \
      format "Mode safety transition {} -> {} at SOC {}"

    telemetry MODE_SAFETY_EPS_VALID: bool id 0x00 update on change
    telemetry MODE_SAFETY_LAST_SOC: F32 id 0x01
    telemetry MODE_SAFETY_LAST_CURRENT_MODE: SatMode id 0x02
    telemetry MODE_SAFETY_LAST_TARGET_MODE: SatMode id 0x03
    telemetry MODE_SAFETY_TRANSITION_COUNT: U32 id 0x04

  }

}
