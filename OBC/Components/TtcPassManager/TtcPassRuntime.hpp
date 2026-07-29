#ifndef OBC_COMPONENTS_TTCPASSMANAGER_TTCPASSRUNTIME_HPP
#define OBC_COMPONENTS_TTCPASSMANAGER_TTCPASSRUNTIME_HPP

#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Components/CommController/CommControllerRuntime.hpp"
#include "OBC/Types/SatModeEnumAc.hpp"
#include "simulators/gps/GpsTypes.hpp"

namespace OBC {

struct TtcPassConfig {
    bool enabled = false;
    U32 lossOfLockTimeoutSec = 0U;
};

struct TtcPassWindow {
    U64 startUnixSec = 0U;
    U64 endUnixSec = 0U;
};

enum class TtcPassPolicyReason : U32 {
    NONE = 0U,
    WINDOW_ACTIVE = 1U,
    TTC_DISABLED = 2U,
    WINDOW_CLEARED = 3U,
    WINDOW_INACTIVE = 4U,
    GPS_INVALID = 5U,
    COMM_LOSS_TIMEOUT = 6U,
};

struct TtcGpsFreshnessTracker {
    bool haveAcceptedSentenceTimestamp = false;
    U32 lastAcceptedSentenceCount = 0U;
    U32 lastAcceptedSentenceWallclockSec = 0U;
};

struct TtcCommLossTracker {
    bool active = false;
    U32 noLinkStartWallclockSec = 0U;
};

struct TtcPassRuntimeStatus {
    OBC::SatMode currentMode = OBC::SatMode::SAFE;
    bool enabled = false;
    U32 lossOfLockTimeoutSec = 0U;
    bool windowConfigured = false;
    U64 windowStartUnixSec = 0U;
    U64 windowEndUnixSec = 0U;
    bool windowActive = false;
    bool gpsTimeValid = false;
    U64 currentGpsUnixSec = 0U;
    bool ttcActive = false;
    U32 lossOfLockTimerSec = 0U;
    OBC::TtcPassPolicyReason lastEntryReason = OBC::TtcPassPolicyReason::NONE;
    OBC::TtcPassPolicyReason lastExitReason = OBC::TtcPassPolicyReason::NONE;
    U32 entryCount = 0U;
    U32 exitCount = 0U;
};

class ITtcPassGpsProvider {
  public:
    virtual ~ITtcPassGpsProvider() = default;

    virtual bool getCachedStateForRuntime(OBC::GPS::StateData& state) const = 0;
};

class ITtcPassCommStateProvider {
  public:
    virtual ~ITtcPassCommStateProvider() = default;

    virtual OBC::CommRuntimeState getStateForRuntime() const = 0;
};

class ITtcPassAdcsControl {
  public:
    virtual ~ITtcPassAdcsControl() = default;

    virtual bool requestPointingForTtcEntry() = 0;
};

inline const char* ttcPassPolicyReasonName(const OBC::TtcPassPolicyReason reason) {
    switch (reason) {
        case OBC::TtcPassPolicyReason::NONE:
            return "NONE";
        case OBC::TtcPassPolicyReason::WINDOW_ACTIVE:
            return "WINDOW_ACTIVE";
        case OBC::TtcPassPolicyReason::TTC_DISABLED:
            return "TTC_DISABLED";
        case OBC::TtcPassPolicyReason::WINDOW_CLEARED:
            return "WINDOW_CLEARED";
        case OBC::TtcPassPolicyReason::WINDOW_INACTIVE:
            return "WINDOW_INACTIVE";
        case OBC::TtcPassPolicyReason::GPS_INVALID:
            return "GPS_INVALID";
        case OBC::TtcPassPolicyReason::COMM_LOSS_TIMEOUT:
            return "COMM_LOSS_TIMEOUT";
        default:
            return "UNKNOWN";
    }
}

inline bool ttcPassAnyLinkAvailable(const OBC::CommRuntimeState& state) {
    return state.sbandAvailable || state.uhfAvailable;
}

}  // namespace OBC

#endif
