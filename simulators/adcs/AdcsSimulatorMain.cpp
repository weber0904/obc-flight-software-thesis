#include "simulators/adcs/AdcsSimServer.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <csignal>
#include <limits>
#include <string>
#include <thread>

namespace {

std::atomic<bool> g_stopRequested(false);

void handleSignal(int) {
    g_stopRequested.store(true);
}

void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0 << " [--node-id <id>] [--control-socket <path>]\n";
}

bool parseNodeId(const char* value, std::uint16_t& nodeId) {
    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed > std::numeric_limits<std::uint16_t>::max()) {
        return false;
    }
    nodeId = static_cast<std::uint16_t>(parsed);
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::uint16_t nodeId = OBC::ADCS::CSP::DEFAULT_ADCS_NODE_ID;
    std::string controlSocketPath;
    for (int i = 1; i < argc; i++) {
        const std::string arg = argv[i];
        if (arg == "--node-id") {
            if (i + 1 >= argc || !parseNodeId(argv[++i], nodeId)) {
                std::cerr << "Invalid or missing ADCS CSP node id\n";
                printUsage(argv[0]);
                return 2;
            }
        } else if (arg == "--control-socket") {
            if (i + 1 >= argc) {
                std::cerr << "Missing ADCS control socket path\n";
                printUsage(argv[0]);
                return 2;
            }
            controlSocketPath = argv[++i];
            if (controlSocketPath.empty()) {
                std::cerr << "ADCS control socket path must not be empty\n";
                printUsage(argv[0]);
                return 2;
            }
        } else {
            std::cerr << "Unknown ADCS simulator argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 2;
        }
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    OBC::ADCS::AdcsSimServer server(nodeId);
    if (!controlSocketPath.empty()) {
        server.configureControlSocket(controlSocketPath);
    }
    if (!server.start()) {
        return 1;
    }

    std::thread serverThread([&server]() { server.run(); });

    while (!g_stopRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server.stop();
    serverThread.join();
    return 0;
}
