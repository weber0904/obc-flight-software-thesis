#include "simulators/eps/EpsSimServer.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace {

std::atomic<bool> g_stopRequested(false);

void handleSignal(int) {
    g_stopRequested.store(true);
}

void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " [--node-id <id>] [--initial-soc <pct>] [--control-socket <path>]\n";
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

bool parseInitialSoc(const char* value, float& soc) {
    char* end = nullptr;
    errno = 0;
    const float parsed = std::strtof(value, &end);
    if (errno != 0 || end == value || *end != '\0' || !std::isfinite(parsed) || parsed < 0.0F ||
        parsed > 100.0F) {
        return false;
    }
    soc = parsed;
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::uint16_t nodeId = OBC::EPS::CSP::DEFAULT_EPS_NODE_ID;
    bool hasInitialSoc = false;
    float initialSoc = 0.0F;
    std::string controlSocketPath;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--node-id") {
            if (i + 1 >= argc || !parseNodeId(argv[++i], nodeId)) {
                std::cerr << "Invalid or missing EPS CSP node id\n";
                printUsage(argv[0]);
                return 2;
            }
        } else if (arg == "--initial-soc") {
            if (i + 1 >= argc || !parseInitialSoc(argv[++i], initialSoc)) {
                std::cerr << "Invalid or missing EPS initial SoC percentage\n";
                printUsage(argv[0]);
                return 2;
            }
            hasInitialSoc = true;
        } else if (arg == "--control-socket") {
            if (i + 1 >= argc) {
                std::cerr << "Missing EPS control socket path\n";
                printUsage(argv[0]);
                return 2;
            }
            controlSocketPath = argv[++i];
            if (controlSocketPath.empty()) {
                std::cerr << "EPS control socket path must not be empty\n";
                printUsage(argv[0]);
                return 2;
            }
        } else {
            std::cerr << "Unknown EPS simulator argument: " << arg << "\n";
            printUsage(argv[0]);
            return 2;
        }
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    OBC::EPS::EpsSimServer server(nodeId);
    if (!controlSocketPath.empty()) {
        server.configureControlSocket(controlSocketPath);
    }
    if (hasInitialSoc) {
        server.applyInitialSoc(initialSoc);
    }
    std::atomic<bool> serverExited(false);
    std::thread serverThread([&server, &serverExited]() {
        server.run();
        serverExited.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (serverExited.load() && !g_stopRequested.load()) {
        serverThread.join();
        return 1;
    }

    while (!g_stopRequested.load() && !serverExited.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server.stop();
    serverThread.join();
    return 0;
}
