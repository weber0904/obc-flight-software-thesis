#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

extern "C" {
#include <csp/csp.h>
}

#include "simulators/csp/CspRuntime.hpp"

namespace {

std::atomic<bool> g_stopRequested(false);

void handleSignal(int) {
    g_stopRequested.store(true);
}

std::uint16_t parsePort(const char* value, std::uint16_t fallback) {
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    // Developer-only peer binary: scripts provide validated numeric ports.
    return static_cast<std::uint16_t>(std::strtoul(value, nullptr, 10));
}

}  // namespace

int main(int argc, char* argv[]) {
    OBC::CSP::RuntimeConfig config = OBC::CSP::runtimeConfigFromEnvironment(2U, "PEERCSP");

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--node-id" && i + 1 < argc) {
            config.nodeId = static_cast<std::uint16_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--transport" && i + 1 < argc) {
            config.transportKind = argv[++i];
        } else if (arg == "--hub-host" && i + 1 < argc) {
            config.hubHost = argv[++i];
        } else if (arg == "--hub-sub-port" && i + 1 < argc) {
            config.hubSubPort = parsePort(argv[++i], config.hubSubPort);
        } else if (arg == "--hub-pub-port" && i + 1 < argc) {
            config.hubPubPort = parsePort(argv[++i], config.hubPubPort);
        } else if (arg == "--can-device" && i + 1 < argc) {
            config.canDevice = argv[++i];
        } else if (arg == "--can-promisc" && i + 1 < argc) {
            config.canPromisc = std::strtoul(argv[++i], nullptr, 10) != 0U;
        } else if (arg == "--interface-name" && i + 1 < argc) {
            config.interfaceName = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 1;
        }
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    OBC::CSP::LibCspRuntime runtime;
    if (runtime.init(config) != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize CSP peer node " << config.nodeId << "\n";
        return 1;
    }

    csp_socket_t socket = {};
    csp_bind(&socket, CSP_ANY);
    csp_listen(&socket, 8);

    while (!g_stopRequested.load()) {
        csp_conn_t* const conn = csp_accept(&socket, 100);
        if (conn == nullptr) {
            continue;
        }

        while (!g_stopRequested.load()) {
            csp_packet_t* const packet = csp_read(conn, 50);
            if (packet == nullptr) {
                break;
            }
            csp_service_handler(packet);
        }

        csp_close(conn);
    }

    runtime.shutdown();
    return 0;
}
