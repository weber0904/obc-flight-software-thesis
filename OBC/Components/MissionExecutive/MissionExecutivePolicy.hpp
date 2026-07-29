#ifndef OBC_COMPONENTS_MISSIONEXECUTIVE_MISSIONEXECUTIVEPOLICY_HPP
#define OBC_COMPONENTS_MISSIONEXECUTIVE_MISSIONEXECUTIVEPOLICY_HPP

#include "OBC/Components/MissionExecutive/MissionExecutiveRuntime.hpp"

namespace OBC {

struct MissionExecutiveStepResult {
    bool configured;
    bool hadValidEpsStatus;
    bool hadValidAdcsStatus;
    bool lowBatteryCondition;
    bool highAngularRateCondition;
    bool lowPowerActive;
    bool detumbleActive;
    bool sunSafePointingActive;
    bool enteredLowPowerThisCycle;
    bool issuedDetumbleThisCycle;
    bool issuedSunSafePointingThisCycle;
    bool adcsCommandErrorThisCycle;
    unsigned int adcsCommandErrorCode;
    float lastSoc;
    float lastAngularRateNorm;
};

class MissionExecutivePolicy {
  public:
    MissionExecutivePolicy();

    void configureRuntimeBindings(ILowPowerModeControl* modeControl,
                                  IEpsAutonomyStatus* epsStatus,
                                  IAdcsAutonomyStatus* adcsStatus,
                                  IAdcsDetumbleControl* adcsDetumbleControl,
                                  IAdcsSunPointingControl* adcsControl);

    MissionExecutiveStepResult step();

    bool isConfigured() const;

    bool isLowPowerActive() const;

    bool isDetumbleActive() const;

    bool isSunSafePointingActive() const;

    static constexpr float LOW_BATTERY_SOC = 20.0F;
    static constexpr float DETUMBLE_RATE_NORM = 0.05F;
    static constexpr double SUN_SAFE_TARGET_Q0 = 0.9238795325;
    static constexpr double SUN_SAFE_TARGET_Q1 = 0.0;
    static constexpr double SUN_SAFE_TARGET_Q2 = 0.3826834323;
    static constexpr double SUN_SAFE_TARGET_Q3 = 0.0;

  private:
    ILowPowerModeControl* m_modeControl;
    IEpsAutonomyStatus* m_epsStatus;
    IAdcsAutonomyStatus* m_adcsStatus;
    IAdcsDetumbleControl* m_adcsDetumbleControl;
    IAdcsSunPointingControl* m_adcsControl;
    bool m_lowPowerActive;
    bool m_detumbleActive;
    bool m_sunSafePointingLatched;
    bool m_detumbleErrorLatched;
    bool m_sunSafeErrorLatched;
};

}  // namespace OBC

#endif
