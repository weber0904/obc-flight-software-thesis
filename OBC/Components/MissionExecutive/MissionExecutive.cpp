#include "OBC/Components/MissionExecutive/MissionExecutive.hpp"

namespace OBC {

MissionExecutive::MissionExecutive(const char* const compName) : MissionExecutiveComponentBase(compName), m_policy() {}

MissionExecutive::~MissionExecutive() = default;

void MissionExecutive::configureRuntimeBindings(ILowPowerModeControl* modeControl,
                                                IEpsAutonomyStatus* epsStatus,
                                                IAdcsAutonomyStatus* adcsStatus,
                                                IAdcsDetumbleControl* adcsDetumbleControl,
                                                IAdcsSunPointingControl* adcsControl) {
    this->m_policy.configureRuntimeBindings(modeControl, epsStatus, adcsStatus, adcsDetumbleControl, adcsControl);
}

MissionExecutiveStepResult MissionExecutive::runCycleForTest() {
    const MissionExecutiveStepResult result = this->m_policy.step();
    this->publishStepResult_(result);
    return result;
}

void MissionExecutive::schedIn_handler(const FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    static_cast<void>(this->runCycleForTest());
}

void MissionExecutive::publishStepResult_(const MissionExecutiveStepResult& result) {
    if (result.hadValidEpsStatus) {
        this->tlmWrite_MISSION_LAST_SOC(result.lastSoc);
    }

    if (result.hadValidAdcsStatus) {
        this->tlmWrite_MISSION_LAST_ANGULAR_RATE_NORM(result.lastAngularRateNorm);
    }

    this->tlmWrite_MISSION_LOW_BATTERY_CONDITION(result.lowBatteryCondition);
    this->tlmWrite_MISSION_HIGH_ANGULAR_RATE_CONDITION(result.highAngularRateCondition);
    this->tlmWrite_MISSION_LOW_POWER_ACTIVE(result.lowPowerActive);
    this->tlmWrite_MISSION_DETUMBLE_ACTIVE(result.detumbleActive);
    this->tlmWrite_MISSION_SUN_SAFE_POINTING_ACTIVE(result.sunSafePointingActive);

    if (result.enteredLowPowerThisCycle) {
        this->log_ACTIVITY_HI_MISSION_LOW_POWER_ENTER(result.lastSoc);
    }

    if (result.issuedDetumbleThisCycle) {
        this->log_ACTIVITY_HI_MISSION_DETUMBLE_COMMAND(result.lastAngularRateNorm);
    }

    if (result.issuedSunSafePointingThisCycle) {
        this->log_ACTIVITY_HI_MISSION_SUN_SAFE_POINTING_COMMAND();
    }

    if (result.adcsCommandErrorThisCycle) {
        this->log_WARNING_HI_MISSION_ADCS_COMMAND_ERROR(result.adcsCommandErrorCode);
    }
}

}  // namespace OBC
