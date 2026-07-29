module OBC {

  passive component OnboardStateMonitor {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    event STATE_MONITOR_UPDATED(healthMask: U32, faultMask: U32, qualityMask: U32) \
      severity activity low \
      id 0x00 \
      format "Onboard reduced state updated health={} fault={} quality={}"

    event STATE_MONITOR_SOURCE_UNAVAILABLE \
      severity warning high \
      id 0x01 \
      format "Onboard state source unavailable"

    telemetry STATE_MONITOR_HAVE_STATE: U8 id 0x00 update on change
    telemetry STATE_MONITOR_HEALTH_MASK: U32 id 0x01
    telemetry STATE_MONITOR_FAULT_MASK: U32 id 0x02
    telemetry STATE_MONITOR_QUALITY_MASK: U32 id 0x03
    telemetry STATE_MONITOR_REDUCTION_COUNT: U32 id 0x04
    telemetry STATE_MONITOR_LAST_SOC: F32 id 0x05
    telemetry STATE_MONITOR_LAST_RATE_NORM: F32 id 0x06

  }

}
