#include <cstdlib>
#include <chrono>
#include <iostream>
#include <thread>

#include "simulators/csp/CspRuntime.hpp"
#include "simulators/eps/EpsTransport.hpp"

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

std::uint16_t parseNodeId(const char* value, std::uint16_t fallback) {
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    return static_cast<std::uint16_t>(std::strtoul(value, nullptr, 10));
}

}  // namespace

int main() {
    const std::uint16_t epsNode =
        parseNodeId(std::getenv("EPS_CSP_NODE_ID"), OBC::EPS::CSP::DEFAULT_EPS_NODE_ID);

    bool pingSuccess = false;
    if (OBC::CSP::defaultRuntime().init(OBC::CSP::runtimeConfigFromEnvironment(1U, "OBCCSP")) !=
        OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "failed to initialize EPS CSP integration client runtime\n";
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (OBC::CSP::defaultRuntime().ping(epsNode, 1000U, pingSuccess) != OBC::CSP::RuntimeStatus::OK ||
        !pingSuccess) {
        std::cerr << "failed to ping EPS CSP node " << epsNode << "\n";
        return 1;
    }

    OBC::EPS::CspEpsTransport transport(epsNode, 1000U);
    OBC::EPS::StatusData status = {};

    if (!expect(transport.getStatus(status) == OBC::EPS::TransportStatus::OK, "EPS_GET_STATUS over CSP failed")) {
        return 1;
    }
    if (!expect(status.soc > 70.0F && status.pdu_status == 0x03U, "unexpected default EPS status")) {
        return 1;
    }

    if (!expect(transport.setPdu(2U, true, status) == OBC::EPS::TransportStatus::OK,
                "EPS_SET_PDU over CSP failed")) {
        return 1;
    }
    if (!expect((status.pdu_status & 0x04U) != 0U, "EPS PDU state did not update over CSP")) {
        return 1;
    }

    if (!expect(transport.setPdu(2U, false, status) == OBC::EPS::TransportStatus::OK,
                "EPS_SET_PDU disable over CSP failed")) {
        return 1;
    }
    if (!expect((status.pdu_status & 0x04U) == 0U, "EPS PDU state did not clear over CSP")) {
        return 1;
    }

    if (!expect(transport.reset(status) == OBC::EPS::TransportStatus::OK, "EPS_RESET over CSP failed")) {
        return 1;
    }
    if (!expect(status.pdu_status == 0x03U, "EPS reset did not restore default PDU state")) {
        return 1;
    }

    const OBC::CSP::RuntimeMetrics metrics = OBC::CSP::defaultRuntime().metrics();
    if (!expect(metrics.txPackets > 0U && metrics.rxPackets > 0U, "CSP metrics did not record EPS traffic")) {
        return 1;
    }

    std::cout << "eps_csp_integration_test: node=" << epsNode << " soc=" << status.soc
              << " pdu=0x" << std::hex << static_cast<int>(status.pdu_status) << std::dec
              << " tx=" << metrics.txPackets << " rx=" << metrics.rxPackets << "\n";
    OBC::CSP::defaultRuntime().shutdown();
    return 0;
}
