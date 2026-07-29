#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "simulators/scenario/ScenarioBridge.hpp"

namespace {

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

OBC::ADCS::CSP::Request makeAdcsModeRequest(std::uint8_t mode) {
    OBC::ADCS::CSP::Request request =
        OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::MODE, 1U);
    request.payload.mode.mode = mode;
    return request;
}

OBC::ADCS::CSP::Request makeAdcsStateRequest(std::uint16_t seq) {
    return OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::STATE, seq);
}

bool writeScenarioFile(const std::string& path) {
    std::ofstream output(path);
    if (!output.is_open()) {
        return false;
    }

    output << "time_sec,sunlight,battery_soc_pct,ground_pass_open,link_available,omega_x_rad_s,omega_y_rad_s,"
              "omega_z_rad_s\n";
    output << "0,1,72,0,0,0.21,-0.11,0.05\n";
    output << "30,0,18,1,1,0.18,-0.09,0.04\n";
    output << "60,1,24,0,0,0.09,-0.04,0.02\n";
    return true;
}

}  // namespace

int main() {
    const std::string scenarioPath = "/tmp/scenario_bridge_integration_test_" + std::to_string(getpid()) + ".csv";
    bool ok = true;

    ok = check(writeScenarioFile(scenarioPath), "Failed to write temporary scenario file") && ok;

    OBC::Scenario::ScenarioTimeline timeline;
    std::string errorMessage;
    ok = check(timeline.loadFromCsvFile(scenarioPath, errorMessage), errorMessage.empty() ? "Scenario load failed" : errorMessage) &&
         ok;
    ok = check(timeline.samples().size() == 3U, "Unexpected replay sample count") && ok;

    const OBC::Scenario::ReplaySample heldSample = timeline.sampleAt(15.0);
    ok = check(heldSample.sunlight == 1U, "Zero-order hold did not keep the first sunlight state") && ok;
    ok = check(approxEqual(heldSample.battery_soc_pct, 72.0F, 1.0e-5), "Zero-order hold did not keep the first battery SoC") &&
         ok;

    OBC::EPS::EpsSimModel epsModel;
    OBC::ADCS::AdcsSimModel adcsModel;
    OBC::Scenario::ScenarioBridge bridge(epsModel, adcsModel, timeline);

    ok = check(bridge.initialize(errorMessage), errorMessage.empty() ? "Scenario bridge initialization failed" : errorMessage) &&
         ok;
    ok = check(bridge.isInitialized(), "Scenario bridge did not report initialized state") && ok;
    ok = check(epsModel.getState().sunlight == 1U, "EPS sunlight was not seeded from the first replay sample") && ok;
    ok = check(approxEqual(epsModel.getState().soc, 72.0F, 1.0e-5), "EPS battery SoC was not seeded from the first replay sample") &&
         ok;
    ok = check(approxEqual(adcsModel.getState().omega_x, 0.21F, 1.0e-5), "ADCS omega_x was not seeded from the first replay sample") &&
         ok;
    ok = check(approxEqual(adcsModel.getState().omega_y, -0.11F, 1.0e-5), "ADCS omega_y was not seeded from the first replay sample") &&
         ok;
    ok = check(approxEqual(adcsModel.getState().omega_z, 0.05F, 1.0e-5), "ADCS omega_z was not seeded from the first replay sample") &&
         ok;

    ok = check(bridge.stepTo(35.0, errorMessage), errorMessage.empty() ? "Scenario replay step failed" : errorMessage) && ok;
    ok = check(epsModel.getState().sunlight == 0U, "EPS sunlight did not update at the replay step") && ok;
    ok = check(approxEqual(epsModel.getState().soc, 18.0F, 1.0e-5), "EPS battery SoC did not update at the replay step") && ok;
    ok = check(bridge.currentSample().ground_pass_open == 1U, "Bridge did not retain ground-pass-open state") && ok;
    ok = check(bridge.currentSample().link_available == 1U, "Bridge did not retain link-availability state") && ok;

    OBC::ADCS::CSP::Request request = makeAdcsModeRequest(1U);
    OBC::ADCS::CSP::Reply response = {};
    ok = check(adcsModel.processCspRequest(request, response) == OBC::ADCS::ResultCode::OK,
               "ADCS DETUMBLE mode request failed after scenario seeding") &&
         ok;

    request = makeAdcsStateRequest(2U);
    for (int i = 0; i < 20; i++) {
        adcsModel.advanceForTest(std::chrono::milliseconds(1000));
        request.header.seq = static_cast<std::uint16_t>(2U + i);
        ok = check(adcsModel.processCspRequest(request, response) == OBC::ADCS::ResultCode::OK,
                   "ADCS state poll failed during detumble replay test") &&
             ok;
    }

    const OBC::ADCS::StateData detumbledState = adcsModel.getState();
    ok = check(omegaNorm(detumbledState) < 0.05F, "ADCS detumble did not converge after scenario seeding") && ok;

    ok = check(bridge.stepTo(60.0, errorMessage), errorMessage.empty() ? "Late scenario replay step failed" : errorMessage) && ok;
    ok = check(approxEqual(adcsModel.getState().omega_x, detumbledState.omega_x, 1.0e-6),
               "Scenario replay unexpectedly overwrote ADCS omega_x after initialization") &&
         ok;
    ok = check(approxEqual(adcsModel.getState().omega_y, detumbledState.omega_y, 1.0e-6),
               "Scenario replay unexpectedly overwrote ADCS omega_y after initialization") &&
         ok;
    ok = check(approxEqual(adcsModel.getState().omega_z, detumbledState.omega_z, 1.0e-6),
               "Scenario replay unexpectedly overwrote ADCS omega_z after initialization") &&
         ok;

    std::remove(scenarioPath.c_str());
    return ok ? 0 : 1;
}
