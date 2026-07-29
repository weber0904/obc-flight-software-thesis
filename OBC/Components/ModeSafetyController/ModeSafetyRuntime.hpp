#ifndef OBC_COMPONENTS_MODESAFETYCONTROLLER_MODESAFETYRUNTIME_HPP
#define OBC_COMPONENTS_MODESAFETYCONTROLLER_MODESAFETYRUNTIME_HPP

#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Types/SatModeEnumAc.hpp"

namespace OBC {
namespace EPS {
struct StatusData;
}

enum class ModeApplySource : U8 {
    SafetyFallback = 1U,
    SafetyRecovery = 2U,
    TestSetup = 3U,
    SafetyPayloadExit = 4U,
    FdirSubsystemFault = 5U,
    WatchdogFault = 6U,
    RecoveryBootFallback = 7U,
    TtcPassPolicy = 8U,
};

enum class ModeTransitionRejectionReason : U32 {
    DISALLOWED_OPERATOR_TRANSITION = 1U,
    INTERNAL_ONLY_TARGET = 2U,
    SOC_GUARD_UNAVAILABLE = 3U,
    SOC_GUARD_NOT_MET = 4U,
    GUARD_UNCONFIGURED = 5U,
};

struct OperatorModeTransitionDecision {
    bool accepted = false;
    OBC::ModeTransitionRejectionReason reason =
        OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION;
};

class IModeOperatorTransitionGuard {
  public:
    virtual ~IModeOperatorTransitionGuard() = default;

    virtual OBC::OperatorModeTransitionDecision evaluateOperatorTransition(OBC::SatMode currentMode,
                                                                           OBC::SatMode requestedMode) const = 0;
};

class IModeSafetyModeControl {
  public:
    virtual ~IModeSafetyModeControl() = default;

    virtual OBC::SatMode getModeForRuntime() const = 0;

    virtual void applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) = 0;
};

class IModeSafetyEpsStatus {
  public:
    virtual ~IModeSafetyEpsStatus() = default;

    virtual bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const = 0;
};

}  // namespace OBC

#endif
