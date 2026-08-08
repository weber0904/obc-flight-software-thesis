#include "simulators/comm/ByteStreamTransport.hpp"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace {

void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " --serial-device <path> [--baudrate <rate>] [--timeout-ms <milliseconds>]"
              << " [--settle-ms <milliseconds>]\n";
}

bool parseU32(const char* value, std::uint32_t& parsedValue) {
    if (value == nullptr || value[0] == '\0' || value[0] == '-') {
        return false;
    }
    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }
    parsedValue = static_cast<std::uint32_t>(parsed);
    return true;
}

const char* byteStreamStatusName(OBC::COMM::ByteStreamStatus status) {
    switch (status) {
        case OBC::COMM::ByteStreamStatus::OK:
            return "OK";
        case OBC::COMM::ByteStreamStatus::TIMEOUT:
            return "TIMEOUT";
        case OBC::COMM::ByteStreamStatus::IO_ERROR:
        default:
            return "IO_ERROR";
    }
}

bool exchange(OBC::COMM::SerialByteStreamTransport& transport,
              const std::string& label,
              const std::string& request,
              std::string& response) {
    response.clear();
    const OBC::COMM::ByteStreamStatus status = transport.exchange(request, response);
    std::cout << label << "-status: " << byteStreamStatusName(status) << "\n";
    if (status == OBC::COMM::ByteStreamStatus::OK) {
        std::cout << label << "-response: " << response << "\n";
        return true;
    }
    return false;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string serialDevice;
    std::uint32_t baudrate = 115200U;
    std::uint32_t timeoutMs = 1000U;
    std::uint32_t settleMs = 200U;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--serial-device") {
            if (i + 1 >= argc) {
                printUsage(argv[0]);
                return 2;
            }
            serialDevice = argv[++i];
        } else if (arg == "--baudrate") {
            if (i + 1 >= argc || !parseU32(argv[++i], baudrate)) {
                std::cerr << "Invalid or missing baudrate\n";
                printUsage(argv[0]);
                return 2;
            }
        } else if (arg == "--timeout-ms") {
            if (i + 1 >= argc || !parseU32(argv[++i], timeoutMs) ||
                timeoutMs > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
                std::cerr << "Invalid or missing timeout (must be <= " << std::numeric_limits<int>::max()
                          << ")\n";
                printUsage(argv[0]);
                return 2;
            }
        } else if (arg == "--settle-ms") {
            if (i + 1 >= argc || !parseU32(argv[++i], settleMs)) {
                std::cerr << "Invalid or missing settle interval\n";
                printUsage(argv[0]);
                return 2;
            }
        } else {
            std::cerr << "Unknown serial link probe argument: " << arg << "\n";
            printUsage(argv[0]);
            return 2;
        }
    }

    if (serialDevice.empty()) {
        std::cerr << "--serial-device is required\n";
        printUsage(argv[0]);
        return 2;
    }

    OBC::COMM::SerialByteStreamTransport transport(serialDevice, timeoutMs, baudrate);
    if (!transport.connect()) {
        std::cerr << "Failed to open serial device\n";
        return 1;
    }
    if (settleMs > 0U) {
        std::this_thread::sleep_for(std::chrono::milliseconds(settleMs));
    }

    std::string initialStatus;
    if (!exchange(transport, "initial-status", "STATUS\n", initialStatus)) {
        std::cerr << "Failed initial STATUS exchange\n";
        return 1;
    }

    std::string enableStatus;
    if (!exchange(transport, "enable", "ENABLE 1\n", enableStatus)) {
        std::cerr << "Failed ENABLE 1 exchange\n";
        return 1;
    }

    std::string finalStatus;
    if (!exchange(transport, "final-status", "STATUS\n", finalStatus)) {
        std::cerr << "Failed final STATUS exchange\n";
        return 1;
    }

    if (finalStatus.find("STATUS enabled=1") == std::string::npos) {
        std::cerr << "Final STATUS response did not report enabled=1\n";
        return 1;
    }

    const OBC::COMM::ByteStreamStats stats = transport.getStats();
    std::cout << "serial-link-probe: PASS\n";
    std::cout << "serial-link-probe-stats:"
              << " connected=" << (stats.connected ? "yes" : "no")
              << " tx=" << stats.txBytes
              << " rx=" << stats.rxBytes
              << " txErr=" << stats.txErrors
              << " rxErr=" << stats.rxErrors
              << "\n";
    return 0;
}
