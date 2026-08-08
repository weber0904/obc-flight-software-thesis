#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "OBC/Components/MissionExecutive/MissionExecutivePolicy.hpp"
#include "simulators/adcs/AdcsSimModel.hpp"
#include "simulators/eps/EpsSimModel.hpp"
#include "simulators/scenario/ScenarioBridge.hpp"

namespace {

constexpr std::uint8_t ADCS_MODE_DETUMBLE = 1U;
constexpr std::uint8_t ADCS_MODE_POINTING = 2U;

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

bool approxEqual(double actual, double expected, double tolerance) {
    return std::fabs(actual - expected) <= tolerance;
}

float omegaNorm(const OBC::ADCS::StateData& state) {
    return std::sqrt(state.omega_x * state.omega_x + state.omega_y * state.omega_y + state.omega_z * state.omega_z);
}

double quaternionDistance(const OBC::ADCS::StateData& lhs, const OBC::ADCS::StateData& rhs) {
    const double dot = lhs.q0 * rhs.q0 + lhs.q1 * rhs.q1 + lhs.q2 * rhs.q2 + lhs.q3 * rhs.q3;
    return 1.0 - std::fabs(dot);
}

bool writeLowBatteryScenarioFile(const std::string& path) {
    std::ofstream output(path);
    if (!output.is_open()) {
        return false;
    }

    output << "time_sec,sunlight,battery_soc_pct,ground_pass_open,link_available,omega_x_rad_s,omega_y_rad_s,"
              "omega_z_rad_s\n";
    output << "0,1,72,0,0,0.03,-0.02,0.01\n";
    output << "30,0,18,0,0,0.03,-0.02,0.01\n";
    return true;
}

bool writeDetumbleScenarioFile(const std::string& path) {
    std::ofstream output(path);
    if (!output.is_open()) {
        return false;
    }

    output << "time_sec,sunlight,battery_soc_pct,ground_pass_open,link_available,omega_x_rad_s,omega_y_rad_s,"
              "omega_z_rad_s\n";
    output << "0,1,72,0,0,0.24,-0.16,0.12\n";
    output << "30,1,72,0,0,0.24,-0.16,0.12\n";
    return true;
}

class FakeModeControl final : public OBC::ILowPowerModeControl {
  public:
    bool isLowPowerModeActiveForRuntime() const override {
        return this->m_lowPowerActive;
    }

    void enterLowPowerModeForRuntime() override {
        this->m_lowPowerActive = true;
        this->m_entryCount++;
    }

    bool m_lowPowerActive = false;
    std::size_t m_entryCount = 0U;
};

class EpsModelStatusView final : public OBC::IEpsAutonomyStatus {
  public:
    explicit EpsModelStatusView(const OBC::EPS::EpsSimModel& model) : m_model(model) {}

    bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const override {
        status = this->m_model.getState();
        return true;
    }

  private:
    const OBC::EPS::EpsSimModel& m_model;
};

class AdcsModelStatusView final : public OBC::IAdcsAutonomyStatus {
  public:
    explicit AdcsModelStatusView(const OBC::ADCS::AdcsSimModel& model) : m_model(model) {}

    bool getCachedStateForRuntime(OBC::ADCS::StateData& state) const override {
        state = this->m_model.getState();
        return true;
    }

  private:
    const OBC::ADCS::AdcsSimModel& m_model;
};

class AdcsModelControl final : public OBC::IAdcsDetumbleControl, public OBC::IAdcsSunPointingControl {
  public:
    explicit AdcsModelControl(OBC::ADCS::AdcsSimModel& model) : m_model(model) {}

    bool commandDetumbleForRuntime() override {
        this->m_detumbleCommandCount++;

        OBC::ADCS::CSP::Request request =
            OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::MODE, 1U);
        OBC::ADCS::CSP::Reply response = {};
        request.payload.mode.mode = ADCS_MODE_DETUMBLE;
        return this->m_model.processCspRequest(request, response) == OBC::ADCS::ResultCode::OK;
    }

    bool commandSunSafePointingForRuntime(double q0, double q1, double q2, double q3) override {
        this->m_pointingCommandCount++;

        OBC::ADCS::CSP::Request request =
            OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::MODE, 1U);
        OBC::ADCS::CSP::Reply response = {};

        request.payload.mode.mode = ADCS_MODE_POINTING;
        if (this->m_model.processCspRequest(request, response) != OBC::ADCS::ResultCode::OK) {
            return false;
        }

        request = OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::TARGET, 2U);
        request.payload.target.q0 = q0;
        request.payload.target.q1 = q1;
        request.payload.target.q2 = q2;
        request.payload.target.q3 = q3;
        if (this->m_model.processCspRequest(request, response) != OBC::ADCS::ResultCode::OK) {
            return false;
        }

        this->m_lastQ0 = q0;
        this->m_lastQ1 = q1;
        this->m_lastQ2 = q2;
        this->m_lastQ3 = q3;
        return true;
    }

    std::size_t m_detumbleCommandCount = 0U;
    std::size_t m_pointingCommandCount = 0U;
    double m_lastQ0 = 0.0;
    double m_lastQ1 = 0.0;
    double m_lastQ2 = 0.0;
    double m_lastQ3 = 0.0;

  private:
    OBC::ADCS::AdcsSimModel& m_model;
};

bool advanceAdcsModel(OBC::ADCS::AdcsSimModel& model,
                      int polls,
                      std::chrono::milliseconds step,
                      const std::string& errorPrefix) {
    OBC::ADCS::CSP::Request request =
        OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::STATE, 1U);
    OBC::ADCS::CSP::Reply response = {};

    for (int i = 0; i < polls; i++) {
        model.advanceForTest(step);
        request.header.seq = static_cast<std::uint16_t>(i + 1);
        if (model.processCspRequest(request, response) != OBC::ADCS::ResultCode::OK) {
            std::cerr << errorPrefix << " state poll failed at index " << i << std::endl;
            return false;
        }
    }

    return true;
}

bool runLowBatteryCase() {
    const std::string scenarioPath = "/tmp/mission_executive_low_battery_" + std::to_string(getpid()) + ".csv";
    bool ok = true;

    ok = check(writeLowBatteryScenarioFile(scenarioPath), "Failed to write low-battery scenario file") && ok;

    OBC::Scenario::ScenarioTimeline timeline;
    std::string errorMessage;
    ok = check(timeline.loadFromCsvFile(scenarioPath, errorMessage), errorMessage.empty() ? "Low-battery scenario load failed"
                                                                                          : errorMessage) &&
         ok;

    OBC::EPS::EpsSimModel epsModel;
    OBC::ADCS::AdcsSimModel adcsModel;
    OBC::Scenario::ScenarioBridge bridge(epsModel, adcsModel, timeline);
    ok = check(bridge.initialize(errorMessage), errorMessage.empty() ? "Low-battery bridge initialization failed"
                                                                     : errorMessage) &&
         ok;

    FakeModeControl modeControl;
    EpsModelStatusView epsView(epsModel);
    AdcsModelStatusView adcsStatus(adcsModel);
    AdcsModelControl adcsControl(adcsModel);
    OBC::MissionExecutivePolicy policy;
    policy.configureRuntimeBindings(&modeControl, &epsView, &adcsStatus, &adcsControl, &adcsControl);

    OBC::MissionExecutiveStepResult result = policy.step();
    ok = check(result.hadValidEpsStatus, "Policy did not consume initial EPS status") && ok;
    ok = check(!result.lowBatteryCondition, "Policy should not trigger at nominal battery") && ok;
    ok = check(!result.highAngularRateCondition, "Policy should not detumble on nominal initial rates") && ok;
    ok = check(adcsControl.m_pointingCommandCount == 0U, "ADCS should not receive pointing command before low battery") && ok;

    ok = check(bridge.stepTo(35.0, errorMessage), errorMessage.empty() ? "Bridge low-battery step failed" : errorMessage) && ok;
    result = policy.step();

    ok = check(result.lowBatteryCondition, "Policy did not detect low battery after replay step") && ok;
    ok = check(result.lowPowerActive, "Policy did not latch low-power state") && ok;
    ok = check(result.sunSafePointingActive, "Policy did not command sun-safe pointing") && ok;
    ok = check(!result.detumbleActive, "Detumble should stay inactive in low-battery pointing case") && ok;
    ok = check(modeControl.m_lowPowerActive, "Mode control did not enter low-power state") && ok;
    ok = check(modeControl.m_entryCount == 1U, "Low-power mode was not entered exactly once") && ok;
    ok = check(adcsControl.m_detumbleCommandCount == 0U, "ADCS should not receive a detumble command in the low-battery case") &&
         ok;
    ok = check(adcsControl.m_pointingCommandCount == 1U, "ADCS did not receive one sun-safe pointing command") && ok;
    ok = check(approxEqual(adcsControl.m_lastQ0, OBC::MissionExecutivePolicy::SUN_SAFE_TARGET_Q0, 1.0e-9),
               "Unexpected sun-safe target q0") &&
         ok;
    ok = check(adcsModel.getState().mode == ADCS_MODE_POINTING,
               "ADCS model did not switch into POINTING mode") &&
         ok;

    const OBC::ADCS::StateData lowBatteryPointingStart = adcsModel.getState();
    ok = check(advanceAdcsModel(adcsModel, 8, std::chrono::milliseconds(1000), "Low-battery"),
               "Low-battery pointing progression failed") &&
         ok;
    const OBC::ADCS::StateData lowBatteryPointingProgressed = adcsModel.getState();
    ok = check(quaternionDistance(lowBatteryPointingStart, lowBatteryPointingProgressed) > 1.0e-4,
               "Low-battery pointing should evolve over simulated time after the autonomy command") &&
         ok;
    ok = check(!approxEqual(lowBatteryPointingStart.pointing_error_deg, lowBatteryPointingProgressed.pointing_error_deg, 1.0e-6),
               "Low-battery pointing error should reflect the evolving synthetic pointing profile") &&
         ok;
    result = policy.step();
    ok = check(!result.highAngularRateCondition,
               "Low-battery pointing should not be misclassified as a high-rate detumble case") &&
         ok;
    ok = check(!result.detumbleActive,
               "Detumble should remain inactive while the low-battery pointing profile is progressing nominally") &&
         ok;

    std::remove(scenarioPath.c_str());
    return ok;
}

bool runDetumbleCase() {
    const std::string scenarioPath = "/tmp/mission_executive_detumble_" + std::to_string(getpid()) + ".csv";
    bool ok = true;

    ok = check(writeDetumbleScenarioFile(scenarioPath), "Failed to write detumble scenario file") && ok;

    OBC::Scenario::ScenarioTimeline timeline;
    std::string errorMessage;
    ok = check(timeline.loadFromCsvFile(scenarioPath, errorMessage), errorMessage.empty() ? "Detumble scenario load failed"
                                                                                           : errorMessage) &&
         ok;

    OBC::EPS::EpsSimModel epsModel;
    OBC::ADCS::AdcsSimModel adcsModel;
    OBC::Scenario::ScenarioBridge bridge(epsModel, adcsModel, timeline);
    ok = check(bridge.initialize(errorMessage), errorMessage.empty() ? "Detumble bridge initialization failed"
                                                                     : errorMessage) &&
         ok;

    FakeModeControl modeControl;
    EpsModelStatusView epsView(epsModel);
    AdcsModelStatusView adcsStatus(adcsModel);
    AdcsModelControl adcsControl(adcsModel);
    OBC::MissionExecutivePolicy policy;
    policy.configureRuntimeBindings(&modeControl, &epsView, &adcsStatus, &adcsControl, &adcsControl);

    OBC::MissionExecutiveStepResult result = policy.step();
    ok = check(result.hadValidAdcsStatus, "Policy did not consume initial ADCS status") && ok;
    ok = check(result.highAngularRateCondition, "Policy did not detect deployment-style high angular rate") && ok;
    ok = check(result.detumbleActive, "Policy did not latch detumble-active state") && ok;
    ok = check(result.issuedDetumbleThisCycle, "Policy did not issue the detumble command") && ok;
    ok = check(!result.sunSafePointingActive, "Sun-safe pointing should stay inactive in the detumble case") && ok;
    ok = check(adcsControl.m_detumbleCommandCount == 1U, "ADCS did not receive one detumble command") && ok;
    ok = check(adcsControl.m_pointingCommandCount == 0U, "ADCS should not receive a pointing command in the detumble case") &&
         ok;
    ok = check(adcsModel.getState().mode == ADCS_MODE_DETUMBLE,
               "ADCS model did not switch into DETUMBLE mode") &&
         ok;
    ok = check(result.lastAngularRateNorm > OBC::MissionExecutivePolicy::DETUMBLE_RATE_NORM,
               "Initial angular-rate norm did not exceed the detumble threshold") &&
         ok;

    ok = check(advanceAdcsModel(adcsModel, 20, std::chrono::milliseconds(1000), "Detumble"),
               "Detumble convergence failed") &&
         ok;
    result = policy.step();

    ok = check(!result.highAngularRateCondition, "High-rate condition should clear after convergence") && ok;
    ok = check(!result.detumbleActive, "Detumble-active state should clear after convergence") && ok;
    ok = check(omegaNorm(adcsModel.getState()) < OBC::MissionExecutivePolicy::DETUMBLE_RATE_NORM,
               "ADCS angular-rate norm did not converge below threshold") &&
         ok;

    std::remove(scenarioPath.c_str());
    return ok;
}

}  // namespace

int main() {
    const bool ok = runLowBatteryCase() && runDetumbleCase();
    return ok ? 0 : 1;
}
