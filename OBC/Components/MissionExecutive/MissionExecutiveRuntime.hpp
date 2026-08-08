#ifndef OBC_COMPONENTS_MISSIONEXECUTIVE_MISSIONEXECUTIVERUNTIME_HPP
#define OBC_COMPONENTS_MISSIONEXECUTIVE_MISSIONEXECUTIVERUNTIME_HPP

#include "simulators/adcs/AdcsTransport.hpp"
#include "simulators/eps/EpsTransport.hpp"

namespace OBC {

class ILowPowerModeControl {
  public:
    virtual ~ILowPowerModeControl() = default;

    virtual bool isLowPowerModeActiveForRuntime() const = 0;

    virtual void enterLowPowerModeForRuntime() = 0;
};

class IEpsAutonomyStatus {
  public:
    virtual ~IEpsAutonomyStatus() = default;

    virtual bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const = 0;
};

class IAdcsSunPointingControl {
  public:
    virtual ~IAdcsSunPointingControl() = default;

    virtual bool commandSunSafePointingForRuntime(double q0, double q1, double q2, double q3) = 0;
};

class IAdcsAutonomyStatus {
  public:
    virtual ~IAdcsAutonomyStatus() = default;

    virtual bool getCachedStateForRuntime(OBC::ADCS::StateData& state) const = 0;
};

class IAdcsDetumbleControl {
  public:
    virtual ~IAdcsDetumbleControl() = default;

    virtual bool commandDetumbleForRuntime() = 0;
};

}  // namespace OBC

#endif
