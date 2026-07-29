#include <cmath>
#include <iostream>

#include "simulators/adcs/AdcsSimModel.hpp"

namespace {

bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

bool approxEqual(double lhs, double rhs, double tolerance) {
    return std::fabs(lhs - rhs) <= tolerance;
}

float omegaNorm(const OBC::ADCS::StateData& state) {
    return std::sqrt(state.omega_x * state.omega_x + state.omega_y * state.omega_y + state.omega_z * state.omega_z);
}

}  // namespace

int main() {
    bool ok = true;

    OBC::ADCS::AdcsSimModel model;
    OBC::ADCS::CSP::Reply reply = {};

    model.advanceForTest(std::chrono::milliseconds(1000));
    const OBC::ADCS::StateData idleState = model.getState();
    const double idleNorm = std::sqrt(idleState.q0 * idleState.q0 + idleState.q1 * idleState.q1 + idleState.q2 * idleState.q2 +
                                      idleState.q3 * idleState.q3);
    ok = check(std::fabs(idleNorm - 1.0) < 1e-6, "idle quaternion should stay normalized") && ok;
    ok = check(!approxEqual(idleState.omega_x, 0.12, 1e-6), "idle omega_x should jitter around baseline") && ok;

    OBC::ADCS::CSP::Request modeRequest =
        OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::MODE, 1U);
    modeRequest.payload.mode.mode = 1U;
    ok = check(model.processCspRequest(modeRequest, reply) == OBC::ADCS::ResultCode::OK,
               "mode request should succeed") &&
         ok;
    const float detumbleNormBefore = omegaNorm(model.getState());
    for (int i = 0; i < 12; i++) {
        model.advanceForTest(std::chrono::milliseconds(1000));
    }
    const OBC::ADCS::StateData detumbleState = model.getState();
    ok = check(omegaNorm(detumbleState) < detumbleNormBefore, "detumble should reduce the angular-rate norm") && ok;
    ok = check(omegaNorm(detumbleState) < 0.05F, "detumble should converge under the mission threshold") && ok;

    OBC::ADCS::AdcsSimModel highRateDetumbleModel;
    OBC::ADCS::CSP::Reply highRateReply = {};
    OBC::ADCS::CSP::Request highRateModeRequest =
        OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::MODE, 10U);
    highRateModeRequest.payload.mode.mode = 1U;
    ok = check(highRateDetumbleModel.processCspRequest(highRateModeRequest, highRateReply) == OBC::ADCS::ResultCode::OK,
               "high-rate detumble mode request should succeed") &&
         ok;
    for (int i = 0; i < 300; i++) {
        highRateDetumbleModel.advanceForTest(std::chrono::milliseconds(50));
    }
    ok = check(omegaNorm(highRateDetumbleModel.getState()) < 0.05F,
               "detumble should still converge under high-rate polling") &&
         ok;

    modeRequest.payload.mode.mode = 2U;
    ok = check(model.processCspRequest(modeRequest, reply) == OBC::ADCS::ResultCode::OK,
               "pointing mode request should succeed") &&
         ok;
    model.advanceForTest(std::chrono::milliseconds(1000));
    const OBC::ADCS::StateData pointingEarly = model.getState();
    ok = check(omegaNorm(pointingEarly) < 0.05F,
               "pointing angular-rate norm should stay below the detumble threshold") &&
         ok;
    model.advanceForTest(std::chrono::milliseconds(20000));
    const OBC::ADCS::StateData pointingLate = model.getState();
    ok = check(!approxEqual(pointingEarly.q0, pointingLate.q0, 1e-6), "pointing pass should keep quaternion moving") && ok;
    ok = check(!approxEqual(pointingEarly.pointing_error_deg, pointingLate.pointing_error_deg, 1e-6),
               "pointing pass should vary pointing error over time") &&
         ok;

    const float latePointingError = pointingLate.pointing_error_deg;
    model.restartPointingPassForRuntime();
    model.advanceForTest(std::chrono::milliseconds(1000));
    const OBC::ADCS::StateData pointingRestarted = model.getState();
    ok = check(pointingRestarted.pointing_error_deg > (latePointingError + 1.0F),
               "restart should rewind the pass enough to create a larger tracking error transient") &&
         ok;
    model.advanceForTest(std::chrono::milliseconds(8000));
    const OBC::ADCS::StateData pointingRecovered = model.getState();
    ok = check(pointingRecovered.pointing_error_deg < pointingRestarted.pointing_error_deg,
               "tracking error should settle again after the restarted pass continues") &&
         ok;

    OBC::ADCS::CSP::Request targetRequest =
        OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::TARGET, 2U);
    targetRequest.payload.target.q0 = 0.9238795325;
    targetRequest.payload.target.q1 = 0.0;
    targetRequest.payload.target.q2 = 0.3826834323;
    targetRequest.payload.target.q3 = 0.0;
    ok = check(model.processCspRequest(targetRequest, reply) == OBC::ADCS::ResultCode::OK,
               "target request should succeed") &&
         ok;

    const OBC::ADCS::StateData mutated = model.getState();
    ok = check(mutated.mode == 2U, "mode should change before reset") && ok;
    ok = check(!approxEqual(mutated.omega_x, 0.12, 1e-6), "omega_x should change before reset") && ok;
    ok = check(!approxEqual(mutated.pointing_error_deg, 0.0, 1e-6), "pointing error should change before reset") && ok;

    const OBC::ADCS::CSP::Request resetRequest =
        OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::RESET, 3U);
    ok = check(model.processCspRequest(resetRequest, reply) == OBC::ADCS::ResultCode::OK,
               "reset request should succeed") &&
         ok;

    const OBC::ADCS::StateData& state = model.getState();
    ok = check(reply.header.service == static_cast<std::uint8_t>(OBC::ADCS::CSP::ServicePort::RESET),
               "reset reply should preserve reset service id") &&
         ok;
    ok = check(state.mode == 0U, "reset should restore IDLE mode") && ok;
    ok = check(state.sensor_valid == 1U, "reset should restore sensor-valid state") && ok;
    ok = check(approxEqual(state.q0, 1.0, 1e-9), "reset should restore q0") && ok;
    ok = check(approxEqual(state.q1, 0.0, 1e-9), "reset should restore q1") && ok;
    ok = check(approxEqual(state.q2, 0.0, 1e-9), "reset should restore q2") && ok;
    ok = check(approxEqual(state.q3, 0.0, 1e-9), "reset should restore q3") && ok;
    ok = check(approxEqual(state.omega_x, 0.12, 1e-6), "reset should restore omega_x") && ok;
    ok = check(approxEqual(state.omega_y, -0.08, 1e-6), "reset should restore omega_y") && ok;
    ok = check(approxEqual(state.omega_z, 0.06, 1e-6), "reset should restore omega_z") && ok;
    ok = check(approxEqual(state.mag_x, 0.222, 1e-6), "reset should restore derived mag_x") && ok;
    ok = check(approxEqual(state.mag_y, -0.048, 1e-6), "reset should restore derived mag_y") && ok;
    ok = check(approxEqual(state.mag_z, 0.446, 1e-6), "reset should restore derived mag_z") && ok;
    ok = check(approxEqual(state.pointing_error_deg, 0.0, 1e-6), "reset should restore pointing error") && ok;
    ok = check(approxEqual(reply.state.q0, state.q0, 1e-9), "reply should return restored state") && ok;

    return ok ? 0 : 1;
}
