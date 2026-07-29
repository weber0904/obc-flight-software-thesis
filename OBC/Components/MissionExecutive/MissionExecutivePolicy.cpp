#include "OBC/Components/MissionExecutive/MissionExecutivePolicy.hpp"

#include <cmath>

namespace OBC {

namespace {

constexpr std::uint8_t ADCS_MODE_DETUMBLE = 1U;

float omegaNorm(const OBC::ADCS::StateData& state) {
    return std::sqrt(state.omega_x * state.omega_x + state.omega_y * state.omega_y + state.omega_z * state.omega_z);
}

}  // namespace

MissionExecutivePolicy::MissionExecutivePolicy()
    : m_modeControl(nullptr),
      m_epsStatus(nullptr),
      m_adcsStatus(nullptr),
      m_adcsDetumbleControl(nullptr),
      m_adcsControl(nullptr),
      m_lowPowerActive(false),
      m_detumbleActive(false),
      m_sunSafePointingLatched(false),
      m_detumbleErrorLatched(false),
      m_sunSafeErrorLatched(false) {}

void MissionExecutivePolicy::configureRuntimeBindings(ILowPowerModeControl* modeControl,
                                                      IEpsAutonomyStatus* epsStatus,
                                                      IAdcsAutonomyStatus* adcsStatus,
                                                      IAdcsDetumbleControl* adcsDetumbleControl,
                                                      IAdcsSunPointingControl* adcsControl) {
    this->m_modeControl = modeControl;
    this->m_epsStatus = epsStatus;
    this->m_adcsStatus = adcsStatus;
    this->m_adcsDetumbleControl = adcsDetumbleControl;
    this->m_adcsControl = adcsControl;
}

MissionExecutiveStepResult MissionExecutivePolicy::step() {
    MissionExecutiveStepResult result = {};
    result.configured = this->isConfigured();
    result.lowPowerActive = this->m_lowPowerActive;
    result.detumbleActive = this->m_detumbleActive;
    result.sunSafePointingActive = this->m_sunSafePointingLatched && !this->m_detumbleActive;

    if (!result.configured) {
        return result;
    }

    OBC::ADCS::StateData adcsState = {};
    if (this->m_adcsStatus->getCachedStateForRuntime(adcsState)) {
        result.hadValidAdcsStatus = true;
        result.lastAngularRateNorm = omegaNorm(adcsState);

        if (result.lastAngularRateNorm > DETUMBLE_RATE_NORM) {
            result.highAngularRateCondition = true;
            this->m_sunSafePointingLatched = false;

            if (adcsState.mode != ADCS_MODE_DETUMBLE) {
                if (this->m_adcsDetumbleControl->commandDetumbleForRuntime()) {
                    result.issuedDetumbleThisCycle = true;
                    this->m_detumbleErrorLatched = false;
                } else if (!this->m_detumbleErrorLatched) {
                    result.adcsCommandErrorThisCycle = true;
                    result.adcsCommandErrorCode = 2U;
                    this->m_detumbleErrorLatched = true;
                }
            }

            this->m_detumbleActive = true;
        } else {
            this->m_detumbleActive = false;
            this->m_detumbleErrorLatched = false;
        }
    } else {
        this->m_detumbleActive = false;
    }

    OBC::EPS::StatusData status = {};
    if (this->m_epsStatus->getCachedStatusForRuntime(status)) {
        result.hadValidEpsStatus = true;
        result.lastSoc = status.soc;

        if (status.soc < LOW_BATTERY_SOC) {
            result.lowBatteryCondition = true;

            if (!this->m_lowPowerActive) {
                if (!this->m_modeControl->isLowPowerModeActiveForRuntime()) {
                    this->m_modeControl->enterLowPowerModeForRuntime();
                }
                this->m_lowPowerActive = true;
                result.enteredLowPowerThisCycle = true;
            }

            if (!result.highAngularRateCondition && !this->m_sunSafePointingLatched) {
                if (this->m_adcsControl->commandSunSafePointingForRuntime(
                        SUN_SAFE_TARGET_Q0, SUN_SAFE_TARGET_Q1, SUN_SAFE_TARGET_Q2, SUN_SAFE_TARGET_Q3)) {
                    this->m_sunSafePointingLatched = true;
                    this->m_sunSafeErrorLatched = false;
                    result.issuedSunSafePointingThisCycle = true;
                } else if (!this->m_sunSafeErrorLatched) {
                    result.adcsCommandErrorThisCycle = true;
                    result.adcsCommandErrorCode = 1U;
                    this->m_sunSafeErrorLatched = true;
                }
            }
        } else {
            this->m_sunSafeErrorLatched = false;
        }
    }

    result.lowPowerActive = this->m_lowPowerActive;
    result.detumbleActive = this->m_detumbleActive;
    result.sunSafePointingActive = this->m_sunSafePointingLatched && !this->m_detumbleActive;
    return result;
}

bool MissionExecutivePolicy::isConfigured() const {
    return this->m_modeControl != nullptr && this->m_epsStatus != nullptr && this->m_adcsStatus != nullptr &&
           this->m_adcsDetumbleControl != nullptr && this->m_adcsControl != nullptr;
}

bool MissionExecutivePolicy::isLowPowerActive() const {
    return this->m_lowPowerActive;
}

bool MissionExecutivePolicy::isDetumbleActive() const {
    return this->m_detumbleActive;
}

bool MissionExecutivePolicy::isSunSafePointingActive() const {
    return this->m_sunSafePointingLatched && !this->m_detumbleActive;
}

}  // namespace OBC
