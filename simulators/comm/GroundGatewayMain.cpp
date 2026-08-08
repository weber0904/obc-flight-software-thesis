#include "simulators/comm/GroundGatewayProxy.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
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
              << " (--serial-device <path> [--baudrate <rate>] | --rf-tcp-port <port> [--rf-tcp-host <host>])"
              << " --gds-port <port> [--gds-host <host>] [--link-identity <name>]"
              << " [--serial-tx-preamble-lines <count>] [--serial-tx-preamble-delay-ms <milliseconds>]"
              << " [--capture-gds-to-southbound <path>] [--capture-southbound-to-gds <path>]\n";
}

bool parseU16(const char* value, std::uint16_t& parsedValue) {
    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed > std::numeric_limits<std::uint16_t>::max()) {
        return false;
    }
    parsedValue = static_cast<std::uint16_t>(parsed);
    return true;
}

bool parseU32(const char* value, std::uint32_t& parsedValue) {
    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }
    parsedValue = static_cast<std::uint32_t>(parsed);
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    OBC::COMM::GroundGatewayConfig config = {};

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--serial-device") {
            if (i + 1 >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            config.southboundMode = OBC::COMM::GroundGatewaySouthboundMode::SERIAL;
            config.serialDevice = argv[++i];
        } else if (arg == "--rf-tcp-host") {
            if (i + 1 >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            config.rfTcpHost = argv[++i];
        } else if (arg == "--rf-tcp-port") {
            if (i + 1 >= argc || !parseU16(argv[++i], config.rfTcpPort)) {
                std::cerr << "Invalid or missing RF TCP port\n";
                printUsage(argv[0]);
                return 2;
            }
            config.southboundMode = OBC::COMM::GroundGatewaySouthboundMode::TCP_CLIENT;
        } else if (arg == "--gds-host") {
            if (i + 1 >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            config.gdsHost = argv[++i];
        } else if (arg == "--gds-port") {
            if (i + 1 >= argc || !parseU16(argv[++i], config.gdsPort)) {
                std::cerr << "Invalid or missing GDS port\n";
                printUsage(argv[0]);
                return 2;
            }
        } else if (arg == "--baudrate") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.baudrate)) {
                std::cerr << "Invalid or missing baudrate\n";
                printUsage(argv[0]);
                return 2;
            }
        } else if (arg == "--serial-tx-preamble-lines") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.serialTxPreambleLines)) {
                std::cerr << "Invalid or missing serial TX preamble line count\n";
                printUsage(argv[0]);
                return 2;
            }
        } else if (arg == "--serial-tx-preamble-delay-ms") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.serialTxPreambleDelayMs)) {
                std::cerr << "Invalid or missing serial TX preamble delay\n";
                printUsage(argv[0]);
                return 2;
            }
        } else if (arg == "--link-identity") {
            if (i + 1 >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            config.linkIdentity = argv[++i];
        } else if (arg == "--capture-gds-to-southbound") {
            if (i + 1 >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            config.captureGdsToSouthboundPath = argv[++i];
        } else if (arg == "--capture-southbound-to-gds") {
            if (i + 1 >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            config.captureSouthboundToGdsPath = argv[++i];
        } else {
            std::cerr << "Unknown ground gateway argument: " << arg << "\n";
            printUsage(argv[0]);
            return 2;
        }
    }

    if (config.gdsPort == 0U) {
        printUsage(argv[0]);
        return 2;
    }
    if (!config.serialDevice.empty() && config.rfTcpPort != 0U) {
        std::cerr << "--serial-device and --rf-tcp-port are mutually exclusive\n";
        printUsage(argv[0]);
        return 2;
    }
    if (config.southboundMode == OBC::COMM::GroundGatewaySouthboundMode::SERIAL && config.serialDevice.empty()) {
        std::cerr << "--serial-device is required for serial gateway mode\n";
        printUsage(argv[0]);
        return 2;
    }
    if (config.southboundMode == OBC::COMM::GroundGatewaySouthboundMode::TCP_CLIENT && config.rfTcpPort == 0U) {
        std::cerr << "--rf-tcp-port must be > 0\n";
        printUsage(argv[0]);
        return 2;
    }

    std::cout << "ground_ttc_gateway startup: link=" << config.linkIdentity
              << " gds=" << config.gdsHost << ":" << config.gdsPort
              << " southbound="
              << (config.southboundMode == OBC::COMM::GroundGatewaySouthboundMode::TCP_CLIENT ? "tcp-client"
                                                                                              : "serial");
    if (config.southboundMode == OBC::COMM::GroundGatewaySouthboundMode::TCP_CLIENT) {
        std::cout << " rf-tcp=" << config.rfTcpHost << ":" << config.rfTcpPort;
    } else {
        std::cout << " serial=" << config.serialDevice
                  << " baudrate=" << config.baudrate
                  << " preamble-lines=" << config.serialTxPreambleLines
                  << " preamble-delay-ms=" << config.serialTxPreambleDelayMs;
    }
    if (!config.captureGdsToSouthboundPath.empty()) {
        std::cout << " capture-gds-to-southbound=" << config.captureGdsToSouthboundPath;
    }
    if (!config.captureSouthboundToGdsPath.empty()) {
        std::cout << " capture-southbound-to-gds=" << config.captureSouthboundToGdsPath;
    }
    std::cout << "\n";
    std::cout.flush();

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    OBC::COMM::GroundGatewayProxy proxy(config);
    std::atomic<bool> proxyExited(false);
    std::thread proxyThread([&proxy, &proxyExited]() {
        proxy.run();
        proxyExited.store(true);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (proxyExited.load() && !g_stopRequested.load()) {
        proxyThread.join();
        return 1;
    }

    while (!g_stopRequested.load() && !proxyExited.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    proxy.stop();
    proxyThread.join();
    return 0;
}
