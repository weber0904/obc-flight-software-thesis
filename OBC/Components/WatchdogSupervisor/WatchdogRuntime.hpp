#ifndef OBC_COMPONENTS_WATCHDOGSUPERVISOR_WATCHDOGRUNTIME_HPP
#define OBC_COMPONENTS_WATCHDOGSUPERVISOR_WATCHDOGRUNTIME_HPP

#include <array>
#include <cstddef>

#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Types/WatchdogRecoveryLevelEnumAc.hpp"
#include "OBC/Types/WatchdogSourceEnumAc.hpp"
#include "OBC/Types/WatchdogStateEnumAc.hpp"

namespace OBC {

struct WatchdogSourceConfig {
    bool enabled = true;
    U32 warningTicks = 2U;
    U32 safeTicks = 3U;
    U32 suppressTicks = 5U;
};

struct WatchdogSourceSnapshot {
    OBC::WatchdogSource source = OBC::WatchdogSource::EPS_BRIDGE;
    OBC::WatchdogState state = OBC::WatchdogState::HEALTHY;
    OBC::WatchdogSourceConfig config = {};
    U32 ageTicks = 0U;
    bool beatPending = false;
    bool probeSuppressed = false;
    U32 lastBeatCode = 0U;
    U32 warningCount = 0U;
    U32 faultCount = 0U;
    U32 suppressionCount = 0U;
    U32 recoveryCount = 0U;
};

struct WatchdogRuntimeSnapshot {
    OBC::WatchdogState aggregateState = OBC::WatchdogState::HEALTHY;
    OBC::WatchdogRecoveryLevel recoveryLevel = OBC::WatchdogRecoveryLevel::NONE;
    bool feedEligible = true;
    U32 warningMask = 0U;
    U32 faultMask = 0U;
    U32 suppressMask = 0U;
    U32 evaluationCount = 0U;
    U32 safeRequestCount = 0U;
    U32 feedSuppressCount = 0U;
    U32 feedStrokeAttemptCount = 0U;
    U32 aggregateRecoveryCount = 0U;
    std::array<OBC::WatchdogSourceSnapshot, OBC::WatchdogSource::NUM_CONSTANTS> sources = {};
};

inline std::size_t watchdogSourceIndex(const OBC::WatchdogSource source) {
    return static_cast<std::size_t>(source.e);
}

inline OBC::WatchdogSource watchdogSourceFromIndex(const FwIndexType portNum) {
    switch (portNum) {
        case 0:
            return OBC::WatchdogSource::EPS_BRIDGE;
        case 1:
            return OBC::WatchdogSource::EPS_FDIR;
        case 2:
            return OBC::WatchdogSource::MODE_SAFETY;
        case 3:
            return OBC::WatchdogSource::COMM_CONTROLLER;
        case 4:
        default:
            return OBC::WatchdogSource::ADCS_FDIR;
    }
}

inline const char* watchdogSourceName(const OBC::WatchdogSource source) {
    switch (source.e) {
        case OBC::WatchdogSource::EPS_BRIDGE:
            return "EPS_BRIDGE";
        case OBC::WatchdogSource::EPS_FDIR:
            return "EPS_FDIR";
        case OBC::WatchdogSource::MODE_SAFETY:
            return "MODE_SAFETY";
        case OBC::WatchdogSource::COMM_CONTROLLER:
            return "COMM_CONTROLLER";
        case OBC::WatchdogSource::ADCS_FDIR:
            return "ADCS_FDIR";
        default:
            return "UNKNOWN";
    }
}

inline U32 watchdogSourceMask(const OBC::WatchdogSource source) {
    return static_cast<U32>(1U << static_cast<U32>(source.e));
}

}  // namespace OBC

#endif
