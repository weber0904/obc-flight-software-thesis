#ifndef OBC_COMPONENTS_WATCHDOGSUPERVISOR_WATCHDOGPOLICY_HPP
#define OBC_COMPONENTS_WATCHDOGSUPERVISOR_WATCHDOGPOLICY_HPP

#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"
#include "OBC/Components/WatchdogSupervisor/WatchdogRuntime.hpp"
#include "OBC/Types/SatModeEnumAc.hpp"

namespace OBC {

struct WatchdogSourceEvaluation {
    U32 nextAgeTicks = 0U;
    OBC::WatchdogState nextState = OBC::WatchdogState::HEALTHY;
    bool crossedWarning = false;
    bool crossedFault = false;
    bool crossedSuppression = false;
    bool recovered = false;
    bool shouldRequestSafe = false;
};

struct WatchdogAggregateRollup {
    OBC::WatchdogState aggregateState = OBC::WatchdogState::HEALTHY;
    OBC::WatchdogRecoveryLevel recoveryLevel = OBC::WatchdogRecoveryLevel::NONE;
    bool feedEligible = true;
    U32 warningMask = 0U;
    U32 faultMask = 0U;
    U32 suppressMask = 0U;
};

class WatchdogPolicy final {
  public:
    static OBC::WatchdogSourceConfig defaultConfig(const OBC::WatchdogSource source);

    static bool isValidConfig(const OBC::WatchdogSourceConfig& config);

    static bool shouldRequestSafeForMode(OBC::SatMode mode);

    static OBC::WatchdogState stateForAge(const OBC::WatchdogSourceConfig& config, U32 ageTicks);

    static OBC::WatchdogSourceEvaluation evaluateSource(const OBC::WatchdogSourceConfig& config,
                                                        const OBC::WatchdogSourceSnapshot& current,
                                                        bool beatObserved,
                                                        OBC::SatMode currentMode);

    static OBC::WatchdogAggregateRollup rollup(
        const std::array<OBC::WatchdogSourceSnapshot, OBC::WatchdogSource::NUM_CONSTANTS>& sources,
        bool safeRequestIssuedForCurrentFault);
};

}  // namespace OBC

#endif
