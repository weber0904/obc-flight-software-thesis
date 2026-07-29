module OBC {

  passive component GroundLinkHealthProvider {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    telemetry GROUND_LINK_HEALTH_S_BAND_AVAILABLE: bool id 0x00 update on change
    telemetry GROUND_LINK_HEALTH_S_BAND_ACTIVITY_AGE_TICKS: U32 id 0x01
    telemetry GROUND_LINK_HEALTH_S_BAND_REASON: U32 id 0x02 update on change

    telemetry GROUND_LINK_HEALTH_UHF_AVAILABLE: bool id 0x03 update on change
    telemetry GROUND_LINK_HEALTH_UHF_ACTIVITY_AGE_TICKS: U32 id 0x04
    telemetry GROUND_LINK_HEALTH_UHF_REASON: U32 id 0x05 update on change
  }

}
