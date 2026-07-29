module OBC {

  passive component MissionExecutive {

    event port Log
    text event port LogText
    telemetry port Tlm
    time get port Time

    sync input port schedIn: Svc.Sched

    event MISSION_LOW_POWER_ENTER(soc: F32) \
      severity activity high \
      id 0x00 \
      format "Mission executive entered LOW_POWER at SOC {}"

    event MISSION_SUN_SAFE_POINTING_COMMAND \
      severity activity high \
      id 0x01 \
      format "Mission executive commanded first-version sun-safe pointing"

    event MISSION_DETUMBLE_COMMAND(rateNorm: F32) \
      severity activity high \
      id 0x02 \
      format "Mission executive commanded DETUMBLE at angular-rate norm {}"

    event MISSION_ADCS_COMMAND_ERROR(code: U32) \
      severity warning high \
      id 0x03 \
      format "Mission executive ADCS autonomy command failed {}"

    telemetry MISSION_LAST_SOC: F32 id 0x00
    telemetry MISSION_LOW_BATTERY_CONDITION: bool id 0x01
    telemetry MISSION_LOW_POWER_ACTIVE: bool id 0x02
    telemetry MISSION_SUN_SAFE_POINTING_ACTIVE: bool id 0x03
    telemetry MISSION_LAST_ANGULAR_RATE_NORM: F32 id 0x04
    telemetry MISSION_HIGH_ANGULAR_RATE_CONDITION: bool id 0x05
    telemetry MISSION_DETUMBLE_ACTIVE: bool id 0x06

  }

}
