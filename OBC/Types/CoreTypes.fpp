module OBC {

  constant CORE_BASE_ID_LOW = 0x10000000
  constant CORE_BASE_ID_HIGH = 0x1000FFFF
  constant PLATFORM_BASE_ID_LOW = 0x10020000
  constant PLATFORM_BASE_ID_HIGH = 0x1002FFFF
  constant APP_BASE_ID_LOW = 0x10030000
  constant APP_BASE_ID_HIGH = 0x1003FFFF
  constant CSP_BASE_ID_LOW = 0x10040000
  constant CSP_BASE_ID_HIGH = 0x1004FFFF
  constant COMM_BASE_ID_LOW = 0x10050000
  constant COMM_BASE_ID_HIGH = 0x1005FFFF
  constant BOOT_BASE_ID_LOW = 0x10060000
  constant BOOT_BASE_ID_HIGH = 0x1006FFFF

  constant OBC_NODE_ID = 1
  constant EPS_NODE_ID = 2
  constant ADCS_NODE_ID = 3
  constant WatchdogSupervisedSourceCount = 5
  constant RecoveryIncidentSourceCount = 10

  enum SatMode : U8 {
    SAFE = 0
    IDLE = 1
    HELL = 2
    PAYLOAD = 3
    TTC = 4
  }

  enum AdcsMode : U8 {
    IDLE = 0
    DETUMBLE = 1
    POINTING = 2
    SLEW = 3
  }

  enum CommBand : U8 {
    SBAND = 0
    UHF = 1
  }

  enum GpsSourceMode : U8 {
    FAKE = 0
    REPLAY = 1
    LIVE_UART = 2
  }

  enum BootSlot : U8 {
    SLOT_A = 0
    SLOT_B = 1
    NONE = 255
  }

  enum HealthItem : U8 {
    CPU_USAGE = 0
    MEM_RSS_MB = 1
    CSP_FREE_BUFFERS = 2
  }

  enum WatchdogSource : U8 {
    EPS_BRIDGE = 0
    EPS_FDIR = 1
    MODE_SAFETY = 2
    COMM_CONTROLLER = 3
    ADCS_FDIR = 4
  }

  enum WatchdogState : U8 {
    DISABLED = 0
    HEALTHY = 1
    WARNING = 2
    LATCHED_FAULT = 3
    FEED_SUPPRESSED = 4
  }

  enum WatchdogRecoveryLevel : U8 {
    NONE = 0
    WARNING = 1
    LATCHED_FAULT = 2
    SAFE_REQUESTED = 3
    FEED_SUPPRESSED = 4
  }

  enum RecoveryIncidentSource : U8 {
    WATCHDOG_EPS_BRIDGE = 0
    WATCHDOG_EPS_FDIR = 1
    WATCHDOG_MODE_SAFETY = 2
    WATCHDOG_COMM_CONTROLLER = 3
    EPS_TIMEOUT = 4
    WATCHDOG_ADCS_FDIR = 5
    ADCS_POLL_TRANSPORT = 6
    ADCS_POLL_FRESHNESS = 7
    COMM_PRIMARY_UNAVAILABLE = 8
    COMM_PRIMARY_TRANSPORT = 9
    NONE = 255
  }

  enum RecoveryLevel : U8 {
    R0_RECORD_ONLY = 0
    R1_RETRY = 1
    R2_RESTART_SOFTWARE_COMPONENT = 2
    R3_RESET_SUBSYSTEM_INTERFACE = 3
    R4_POWER_CYCLE_SUBSYSTEM = 4
    R5_MODE_FALLBACK = 5
    R6_OBC_REBOOT = 6
    R7_ENTER_HELL = 7
  }

  enum RecoveryAction : U8 {
    NONE = 0
    PROCESS_RESTART_INTENT = 1
    SUBSYSTEM_INTERFACE_RESET = 2
    SAFE_FALLBACK = 3
    OBC_REBOOT = 4
    INCIDENT_CLEARED = 5
    COMM_LINK_FAILOVER = 6
    PROCESS_RESTART = 7
  }

  enum ResetCause : U8 {
    UNKNOWN = 0
    RECOVERY_WATCHDOG = 1
    RECOVERY_EPS_TIMEOUT = 2
    RECOVERY_ADCS_FDIR = 3
    RECOVERY_COMM_FDIR = 4
  }

  enum PersistentFaultRecordKind : U8 {
    BOOT_OBSERVED = 0
    RECOVERY_BOOT_ACK = 1
    INCIDENT_OPENED = 2
    ACTION_REQUESTED = 3
    ACTION_EXECUTED = 4
    REBOOT_PENDING = 5
    REBOOT_ISSUED = 6
    INCIDENT_CLEARED = 7
  }

  enum PersistentFaultStoreCopy : U8 {
    NONE = 0
    COPY_A = 1
    COPY_B = 2
  }

}
