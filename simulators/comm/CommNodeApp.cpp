#include "simulators/comm/CommNodeApp.hpp"

#include "simulators/comm/CommNodeServer.hpp"

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

void printUsage(const char* argv0, const OBC::COMM::CommNodeAppDefaults& defaults) {
    std::cerr << "Usage: " << argv0
              << " (--serial-device <path> [--baudrate <rate>] | --tcp-listen-port <port> [--tcp-listen-host <host>])"
              << " [--node-id <id>] [--interface-name <name>]"
              << " [--beacon-serial-device <path>] [--beacon-baudrate <rate>]\n"
              << "Defaults: executable=" << defaults.executableName
              << " link=" << defaults.linkIdentity
              << " node=" << defaults.nodeId
              << " interface=" << defaults.interfaceName << "\n";
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

namespace OBC {
namespace COMM {

int runCommNodeApp(int argc, char* argv[], const CommNodeAppDefaults& defaults) {
    g_stopRequested.store(false);

    CommNodeConfig config = {};
    config.nodeId = defaults.nodeId;
    config.interfaceName = defaults.interfaceName == nullptr ? "" : defaults.interfaceName;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--serial-device") {
            if (i + 1 >= argc) {
                printUsage(argv[0], defaults);
                return 2;
            }
            config.linkMode = CommExternalLinkMode::SERIAL;
            config.serialDevice = argv[++i];
        } else if (arg == "--tcp-listen-host") {
            if (i + 1 >= argc) {
                printUsage(argv[0], defaults);
                return 2;
            }
            config.tcpListenHost = argv[++i];
        } else if (arg == "--tcp-listen-port") {
            if (i + 1 >= argc || !parseU16(argv[++i], config.tcpListenPort)) {
                std::cerr << "Invalid or missing TCP listen port\n";
                printUsage(argv[0], defaults);
                return 2;
            }
            config.linkMode = CommExternalLinkMode::TCP_SERVER;
        } else if (arg == "--baudrate") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.baudrate)) {
                std::cerr << "Invalid or missing baudrate\n";
                printUsage(argv[0], defaults);
                return 2;
            }
        } else if (arg == "--beacon-serial-device") {
            if (i + 1 >= argc) {
                printUsage(argv[0], defaults);
                return 2;
            }
            config.beaconSerialDevice = argv[++i];
        } else if (arg == "--beacon-baudrate") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.beaconBaudrate)) {
                std::cerr << "Invalid or missing beacon baudrate\n";
                printUsage(argv[0], defaults);
                return 2;
            }
        } else if (arg == "--node-id") {
            if (i + 1 >= argc || !parseU16(argv[++i], config.nodeId)) {
                std::cerr << "Invalid or missing COMM CSP node id\n";
                printUsage(argv[0], defaults);
                return 2;
            }
        } else if (arg == "--interface-name") {
            if (i + 1 >= argc) {
                printUsage(argv[0], defaults);
                return 2;
            }
            config.interfaceName = argv[++i];
        } else {
            std::cerr << "Unknown COMM node argument: " << arg << "\n";
            printUsage(argv[0], defaults);
            return 2;
        }
    }

    if (!config.serialDevice.empty() && config.tcpListenPort != 0U) {
        std::cerr << "--serial-device and --tcp-listen-port are mutually exclusive\n";
        printUsage(argv[0], defaults);
        return 2;
    }
    if (config.linkMode == CommExternalLinkMode::SERIAL && config.serialDevice.empty()) {
        std::cerr << "--serial-device is required for serial COMM node mode\n";
        printUsage(argv[0], defaults);
        return 2;
    }
    if (config.linkMode == CommExternalLinkMode::TCP_SERVER && config.tcpListenPort == 0U) {
        std::cerr << "--tcp-listen-port must be > 0\n";
        printUsage(argv[0], defaults);
        return 2;
    }
    if (!config.beaconSerialDevice.empty() && config.beaconBaudrate == 0U) {
        std::cerr << "--beacon-baudrate must be > 0\n";
        printUsage(argv[0], defaults);
        return 2;
    }

    std::cout << "COMM node startup: executable=" << defaults.executableName
              << " link=" << defaults.linkIdentity
              << " node=" << config.nodeId
              << " endpoint="
              << (config.linkMode == CommExternalLinkMode::TCP_SERVER ? "tcp-listen" : "serial");
    if (config.linkMode == CommExternalLinkMode::TCP_SERVER) {
        std::cout << " tcp=" << config.tcpListenHost << ":" << config.tcpListenPort;
    } else {
        std::cout << " serial=" << config.serialDevice
                  << " baudrate=" << config.baudrate;
    }
    std::cout << " interface=" << config.interfaceName;
    if (!config.beaconSerialDevice.empty()) {
        std::cout << " beacon-serial=" << config.beaconSerialDevice
                  << " beacon-baudrate=" << config.beaconBaudrate;
    } else {
        std::cout << " beacon-serial=disabled";
    }
    std::cout << "\n";
    std::cout.flush();

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    CommNodeServer server(config);
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

}  // namespace COMM
}  // namespace OBC
