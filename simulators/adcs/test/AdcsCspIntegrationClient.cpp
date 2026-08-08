#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "simulators/adcs/AdcsTransport.hpp"
#include "simulators/csp/CspRuntime.hpp"

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

bool approxEqual(double lhs, double rhs, double tolerance) {
    return std::fabs(lhs - rhs) <= tolerance;
}

std::uint16_t parseNodeId(const char* value, std::uint16_t fallback) {
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    return static_cast<std::uint16_t>(std::strtoul(value, nullptr, 10));
}

float omegaNorm(const OBC::ADCS::StateData& state) {
    return std::sqrt(state.omega_x * state.omega_x + state.omega_y * state.omega_y + state.omega_z * state.omega_z);
}

}  // namespace

int main() {
    const std::uint16_t adcsNode =
        parseNodeId(std::getenv("ADCS_CSP_NODE_ID"), OBC::ADCS::CSP::DEFAULT_ADCS_NODE_ID);

    if (OBC::CSP::defaultRuntime().init(OBC::CSP::runtimeConfigFromEnvironment(1U, "OBCCSP")) !=
        OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "failed to initialize ADCS CSP integration client runtime\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    bool pingSuccess = false;
    if (OBC::CSP::defaultRuntime().ping(adcsNode, 1000U, pingSuccess) != OBC::CSP::RuntimeStatus::OK ||
        !pingSuccess) {
        std::cerr << "failed to ping ADCS CSP node " << adcsNode << "\n";
        return 1;
    }

    OBC::ADCS::CspAdcsTransport transport(adcsNode, 1000U);
    OBC::ADCS::StateData state = {};

    if (!expect(transport.getState(state) == OBC::ADCS::TransportStatus::OK, "ADCS_GET_STATE over CSP failed")) {
        return 1;
    }
    if (!expect(state.mode == 0U && state.sensor_valid == 1U, "unexpected default ADCS state")) {
        return 1;
    }

    if (!expect(transport.setMode(1U, state) == OBC::ADCS::TransportStatus::OK,
                "ADCS_SET_MODE DETUMBLE over CSP failed")) {
        return 1;
    }
    for (int i = 0; i < 4; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (!expect(transport.getState(state) == OBC::ADCS::TransportStatus::OK, "ADCS detumble poll failed")) {
            return 1;
        }
    }
    if (!expect(omegaNorm(state) < 0.05F, "ADCS detumble state did not converge over CSP")) {
        return 1;
    }

    if (!expect(transport.setMode(2U, state) == OBC::ADCS::TransportStatus::OK,
                "ADCS_SET_MODE POINTING over CSP failed")) {
        return 1;
    }
    if (!expect(transport.setTarget(0.9238795325, 0.0, 0.3826834323, 0.0, state) ==
                    OBC::ADCS::TransportStatus::OK,
                "ADCS_SET_TARGET over CSP failed")) {
        return 1;
    }
    const OBC::ADCS::StateData pointingFirst = state;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    if (!expect(transport.getState(state) == OBC::ADCS::TransportStatus::OK, "ADCS pointing poll failed")) {
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    OBC::ADCS::StateData pointingSecond = {};
    if (!expect(transport.getState(pointingSecond) == OBC::ADCS::TransportStatus::OK, "ADCS pointing follow-up poll failed")) {
        return 1;
    }
    if (!expect(!approxEqual(pointingFirst.q0, pointingSecond.q0, 1e-6) ||
                    !approxEqual(pointingFirst.q1, pointingSecond.q1, 1e-6) ||
                    !approxEqual(pointingFirst.q2, pointingSecond.q2, 1e-6) ||
                    !approxEqual(pointingFirst.q3, pointingSecond.q3, 1e-6),
                "ADCS pointing state did not evolve over CSP")) {
        return 1;
    }
    if (!expect(!approxEqual(pointingFirst.pointing_error_deg, pointingSecond.pointing_error_deg, 1e-6),
                "ADCS pointing error did not evolve over CSP")) {
        return 1;
    }
    state = pointingSecond;

    if (!expect(transport.calibrate(0U, state) == OBC::ADCS::TransportStatus::OK,
                "ADCS_CALIBRATE over CSP failed")) {
        return 1;
    }
    if (!expect(state.sensor_valid == 1U, "ADCS calibration did not preserve sensor-valid state")) {
        return 1;
    }

    if (!expect(transport.reset(state) == OBC::ADCS::TransportStatus::OK, "ADCS_RESET over CSP failed")) {
        return 1;
    }
    if (!expect(state.mode == 0U, "ADCS reset did not restore IDLE mode")) {
        return 1;
    }
    if (!expect(state.sensor_valid == 1U, "ADCS reset did not restore sensor-valid state")) {
        return 1;
    }
    if (!expect(approxEqual(state.q0, 1.0, 1e-9) && approxEqual(state.q1, 0.0, 1e-9) &&
                    approxEqual(state.q2, 0.0, 1e-9) && approxEqual(state.q3, 0.0, 1e-9),
                "ADCS reset did not restore identity quaternion")) {
        return 1;
    }
    if (!expect(approxEqual(state.omega_x, 0.12, 1e-6) && approxEqual(state.omega_y, -0.08, 1e-6) &&
                    approxEqual(state.omega_z, 0.06, 1e-6),
                "ADCS reset did not restore default angular rates")) {
        return 1;
    }
    if (!expect(approxEqual(state.pointing_error_deg, 0.0, 1e-6),
                "ADCS reset did not restore default pointing error")) {
        return 1;
    }

    const OBC::CSP::RuntimeMetrics metrics = OBC::CSP::defaultRuntime().metrics();
    if (!expect(metrics.txPackets > 0U && metrics.rxPackets > 0U, "CSP metrics did not record ADCS traffic")) {
        return 1;
    }

    std::cout << "adcs_csp_integration_test: node=" << adcsNode << " mode=" << static_cast<int>(state.mode)
              << " pointing_error_deg=" << state.pointing_error_deg << " tx=" << metrics.txPackets
              << " rx=" << metrics.rxPackets << "\n";
    OBC::CSP::defaultRuntime().shutdown();
    return 0;
}
