#ifndef OBC_COMPONENTS_RECOVERYEXECUTOR_RECOVERYRUNTIME_HPP
#define OBC_COMPONENTS_RECOVERYEXECUTOR_RECOVERYRUNTIME_HPP

#include <array>
#include <cstddef>

#include "Fw/Cmd/CmdResponseEnumAc.hpp"
#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Types/FppConstantsAc.hpp"
#include "OBC/Types/CommBandEnumAc.hpp"
#include "OBC/Types/RecoveryActionEnumAc.hpp"
#include "OBC/Types/RecoveryIncidentSourceEnumAc.hpp"
#include "OBC/Types/RecoveryLevelEnumAc.hpp"
#include "OBC/Types/ResetCauseEnumAc.hpp"
#include "OBC/Types/WatchdogSourceEnumAc.hpp"

namespace OBC {

namespace EPS {
struct StatusData;
}

namespace ADCS {
struct StateData;
}

struct RecoveryIncidentState {
    bool active = false;
    bool awaitingClear = false;
    bool hasOpenedBefore = false;
    bool firstActionIssued = false;
    bool restartIntentIssued = false;
    bool processRestartPending = false;
    bool safeFallbackIssued = false;
    bool resetIssued = false;
    bool commFailoverIssued = false;
    bool rebootPending = false;
    OBC::RecoveryIncidentSource source = OBC::RecoveryIncidentSource::NONE;
    OBC::RecoveryLevel currentLevel = OBC::RecoveryLevel::R0_RECORD_ONLY;
    OBC::RecoveryLevel highestLevel = OBC::RecoveryLevel::R0_RECORD_ONLY;
    OBC::RecoveryAction lastAction = OBC::RecoveryAction::NONE;
    U32 epoch = 0U;
    U32 relatchCount = 0U;
    U32 openTicks = 0U;
};

struct RecoveryRuntimeStatus {
    U32 activeIncidentCount = 0U;
    OBC::RecoveryIncidentSource activeSource = OBC::RecoveryIncidentSource::NONE;
    OBC::RecoveryLevel currentLevel = OBC::RecoveryLevel::R0_RECORD_ONLY;
    OBC::RecoveryLevel highestLevel = OBC::RecoveryLevel::R0_RECORD_ONLY;
    OBC::RecoveryAction lastAction = OBC::RecoveryAction::NONE;
    bool pendingProcessRestart = false;
    bool pendingReboot = false;
    U32 relatchCount = 0U;
    U32 totalSafeFallbacks = 0U;
    U32 totalResetActions = 0U;
    U32 totalProcessRestarts = 0U;
    U32 totalReboots = 0U;
    std::array<OBC::RecoveryIncidentState, OBC::RecoveryIncidentSourceCount> incidents = {};
};

enum class RecoveryExitRequest {
    NONE,
    PROCESS_RESTART,
    OBC_REBOOT,
};

struct RecoveryCommActionResult {
    bool switched = false;
    bool alreadyOnHealthyPrimary = false;
    bool noHealthyBackup = false;
    U32 sessionsRevoked = 0U;
    U32 ownersCleared = 0U;
    OBC::CommBand finalPrimaryCommandLink = OBC::CommBand::SBAND;
    OBC::CommBand finalPrimaryTelemetryLink = OBC::CommBand::SBAND;
    OBC::CommBand finalPrimaryFileLink = OBC::CommBand::SBAND;
};

class IRecoveryEpsControl {
  public:
    virtual ~IRecoveryEpsControl() = default;
    virtual Fw::CmdResponse resetEpsForRecovery(OBC::EPS::StatusData& status) = 0;
};

class IRecoveryAdcsControl {
  public:
    virtual ~IRecoveryAdcsControl() = default;
    virtual Fw::CmdResponse resetAdcsForRecovery(OBC::ADCS::StateData& state) = 0;
};

class IRecoveryBootControl {
  public:
    virtual ~IRecoveryBootControl() = default;
    virtual bool isBootSafeFallbackRequiredForRuntime() const = 0;
    virtual U32 getBootCountForRuntime() const = 0;
    virtual U32 getConsecutiveResetCountForRuntime() const = 0;
    virtual U32 getUptimeForRuntime() const = 0;
    virtual bool recordRecoveryRestartIntentForRuntime(OBC::ResetCause cause,
                                                       OBC::RecoveryIncidentSource source,
                                                       OBC::RecoveryLevel level) = 0;
    virtual bool recordRecoveryRebootIntentForRuntime(OBC::ResetCause cause,
                                                      OBC::RecoveryIncidentSource source,
                                                      OBC::RecoveryLevel level) {
        return this->recordRecoveryRestartIntentForRuntime(cause, source, level);
    }
    virtual bool acknowledgeRuntimeStableForRuntime() = 0;
};

class IRecoveryCommControl {
  public:
    virtual ~IRecoveryCommControl() = default;
    virtual OBC::RecoveryCommActionResult performRecoveryLinkFailoverForRuntime() = 0;
};

class IRecoveryRequestSink {
  public:
    virtual ~IRecoveryRequestSink() = default;

    virtual void submitWatchdogFault(OBC::WatchdogSource source) = 0;
    virtual void submitWatchdogSuppression(OBC::WatchdogSource source) = 0;
    virtual void clearWatchdogFault(OBC::WatchdogSource source) = 0;
    virtual void submitEpsTimeoutFault(U32 failureCount) = 0;
    virtual void clearEpsTimeoutFault(U32 failureCount) = 0;
    virtual void submitAdcsPollTransportFault(U32 failureCount) = 0;
    virtual void clearAdcsPollTransportFault(U32 failureCount) = 0;
    virtual void submitAdcsPollFreshnessFault(U32 failureCount) = 0;
    virtual void clearAdcsPollFreshnessFault(U32 failureCount) = 0;
    virtual void submitCommPrimaryUnavailableFault(U32 failureCount) = 0;
    virtual void clearCommPrimaryUnavailableFault(U32 failureCount) = 0;
    virtual void submitCommPrimaryTransportFault(U32 failureCount) = 0;
    virtual void clearCommPrimaryTransportFault(U32 failureCount) = 0;
};

static_assert(static_cast<U32>(OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT) + 1U ==
                  OBC::RecoveryIncidentSourceCount,
              "RecoveryIncidentSourceCount must track the highest indexed recovery source");
static_assert(static_cast<U32>(OBC::WatchdogSource::ADCS_FDIR) + 1U == OBC::WatchdogSupervisedSourceCount,
              "WatchdogSupervisedSourceCount must track the highest supervised watchdog source");

inline bool recoveryIncidentSourceFromWatchdog(const OBC::WatchdogSource source, OBC::RecoveryIncidentSource& out) {
    switch (source.e) {
        case OBC::WatchdogSource::EPS_BRIDGE:
            out = OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE;
            return true;
        case OBC::WatchdogSource::EPS_FDIR:
            out = OBC::RecoveryIncidentSource::WATCHDOG_EPS_FDIR;
            return true;
        case OBC::WatchdogSource::MODE_SAFETY:
            out = OBC::RecoveryIncidentSource::WATCHDOG_MODE_SAFETY;
            return true;
        case OBC::WatchdogSource::COMM_CONTROLLER:
            out = OBC::RecoveryIncidentSource::WATCHDOG_COMM_CONTROLLER;
            return true;
        case OBC::WatchdogSource::ADCS_FDIR:
            out = OBC::RecoveryIncidentSource::WATCHDOG_ADCS_FDIR;
            return true;
        default:
            out = OBC::RecoveryIncidentSource::NONE;
            return false;
    }
}

inline bool recoveryIncidentSourceIsWatchdog(const OBC::RecoveryIncidentSource source) {
    switch (source.e) {
        case OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE:
        case OBC::RecoveryIncidentSource::WATCHDOG_EPS_FDIR:
        case OBC::RecoveryIncidentSource::WATCHDOG_MODE_SAFETY:
        case OBC::RecoveryIncidentSource::WATCHDOG_COMM_CONTROLLER:
        case OBC::RecoveryIncidentSource::WATCHDOG_ADCS_FDIR:
            return true;
        default:
            return false;
    }
}

inline bool recoveryIncidentSourceIsIndexed(const OBC::RecoveryIncidentSource source) {
    return static_cast<U32>(source.e) < OBC::RecoveryIncidentSourceCount;
}

inline std::size_t recoveryIncidentIndex(const OBC::RecoveryIncidentSource source) {
    return static_cast<std::size_t>(source.e);
}

inline const char* recoveryIncidentSourceName(const OBC::RecoveryIncidentSource source) {
    switch (source.e) {
        case OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE:
            return "WATCHDOG_EPS_BRIDGE";
        case OBC::RecoveryIncidentSource::WATCHDOG_EPS_FDIR:
            return "WATCHDOG_EPS_FDIR";
        case OBC::RecoveryIncidentSource::WATCHDOG_MODE_SAFETY:
            return "WATCHDOG_MODE_SAFETY";
        case OBC::RecoveryIncidentSource::WATCHDOG_COMM_CONTROLLER:
            return "WATCHDOG_COMM_CONTROLLER";
        case OBC::RecoveryIncidentSource::EPS_TIMEOUT:
            return "EPS_TIMEOUT";
        case OBC::RecoveryIncidentSource::WATCHDOG_ADCS_FDIR:
            return "WATCHDOG_ADCS_FDIR";
        case OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT:
            return "ADCS_POLL_TRANSPORT";
        case OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS:
            return "ADCS_POLL_FRESHNESS";
        case OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE:
            return "COMM_PRIMARY_UNAVAILABLE";
        case OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT:
            return "COMM_PRIMARY_TRANSPORT";
        default:
            return "NONE";
    }
}

inline const char* recoveryLevelName(const OBC::RecoveryLevel level) {
    switch (level.e) {
        case OBC::RecoveryLevel::R0_RECORD_ONLY:
            return "R0_RECORD_ONLY";
        case OBC::RecoveryLevel::R1_RETRY:
            return "R1_RETRY";
        case OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT:
            return "R2_RESTART_SOFTWARE_COMPONENT";
        case OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE:
            return "R3_RESET_SUBSYSTEM_INTERFACE";
        case OBC::RecoveryLevel::R4_POWER_CYCLE_SUBSYSTEM:
            return "R4_POWER_CYCLE_SUBSYSTEM";
        case OBC::RecoveryLevel::R5_MODE_FALLBACK:
            return "R5_MODE_FALLBACK";
        case OBC::RecoveryLevel::R6_OBC_REBOOT:
            return "R6_OBC_REBOOT";
        case OBC::RecoveryLevel::R7_ENTER_HELL:
            return "R7_ENTER_HELL";
        default:
            return "UNKNOWN";
    }
}

inline const char* recoveryActionName(const OBC::RecoveryAction action) {
    switch (action.e) {
        case OBC::RecoveryAction::PROCESS_RESTART_INTENT:
            return "PROCESS_RESTART_INTENT";
        case OBC::RecoveryAction::SUBSYSTEM_INTERFACE_RESET:
            return "SUBSYSTEM_INTERFACE_RESET";
        case OBC::RecoveryAction::SAFE_FALLBACK:
            return "SAFE_FALLBACK";
        case OBC::RecoveryAction::OBC_REBOOT:
            return "OBC_REBOOT";
        case OBC::RecoveryAction::INCIDENT_CLEARED:
            return "INCIDENT_CLEARED";
        case OBC::RecoveryAction::COMM_LINK_FAILOVER:
            return "COMM_LINK_FAILOVER";
        case OBC::RecoveryAction::PROCESS_RESTART:
            return "PROCESS_RESTART";
        default:
            return "NONE";
    }
}

inline OBC::RecoveryLevel recoveryInitialLevelForSource(const OBC::RecoveryIncidentSource source) {
    switch (source.e) {
        case OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE:
        case OBC::RecoveryIncidentSource::WATCHDOG_EPS_FDIR:
        case OBC::RecoveryIncidentSource::WATCHDOG_MODE_SAFETY:
        case OBC::RecoveryIncidentSource::WATCHDOG_COMM_CONTROLLER:
        case OBC::RecoveryIncidentSource::WATCHDOG_ADCS_FDIR:
            return OBC::RecoveryLevel::R2_RESTART_SOFTWARE_COMPONENT;
        case OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT:
        case OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS:
        case OBC::RecoveryIncidentSource::EPS_TIMEOUT:
        case OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE:
        case OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT:
            return OBC::RecoveryLevel::R3_RESET_SUBSYSTEM_INTERFACE;
        default:
            return OBC::RecoveryLevel::R0_RECORD_ONLY;
    }
}

inline OBC::ResetCause recoveryResetCauseForSource(const OBC::RecoveryIncidentSource source) {
    switch (source.e) {
        case OBC::RecoveryIncidentSource::EPS_TIMEOUT:
            return OBC::ResetCause::RECOVERY_EPS_TIMEOUT;
        case OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT:
        case OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS:
            return OBC::ResetCause::RECOVERY_ADCS_FDIR;
        case OBC::RecoveryIncidentSource::COMM_PRIMARY_UNAVAILABLE:
        case OBC::RecoveryIncidentSource::COMM_PRIMARY_TRANSPORT:
            return OBC::ResetCause::RECOVERY_COMM_FDIR;
        case OBC::RecoveryIncidentSource::WATCHDOG_EPS_BRIDGE:
        case OBC::RecoveryIncidentSource::WATCHDOG_EPS_FDIR:
        case OBC::RecoveryIncidentSource::WATCHDOG_MODE_SAFETY:
        case OBC::RecoveryIncidentSource::WATCHDOG_COMM_CONTROLLER:
        case OBC::RecoveryIncidentSource::WATCHDOG_ADCS_FDIR:
            return OBC::ResetCause::RECOVERY_WATCHDOG;
        default:
            return OBC::ResetCause::UNKNOWN;
    }
}

inline const char* resetCauseName(const OBC::ResetCause cause) {
    switch (cause.e) {
        case OBC::ResetCause::RECOVERY_WATCHDOG:
            return "RECOVERY_WATCHDOG";
        case OBC::ResetCause::RECOVERY_EPS_TIMEOUT:
            return "RECOVERY_EPS_TIMEOUT";
        case OBC::ResetCause::RECOVERY_ADCS_FDIR:
            return "RECOVERY_ADCS_FDIR";
        case OBC::ResetCause::RECOVERY_COMM_FDIR:
            return "RECOVERY_COMM_FDIR";
        default:
            return "UNKNOWN";
    }
}

}  // namespace OBC

#endif
