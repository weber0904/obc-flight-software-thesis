#include "simulators/comm/CommCspProtocol.hpp"
#include "simulators/csp/CspRuntime.hpp"

#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace {

struct Config {
    std::uint16_t localNode = 7U;
    std::uint16_t targetNode = OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID;
    std::uint32_t timeoutMs = 1000U;
    std::string interfaceName = "DLV2PRB";
};

void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " [--target-node <id>] [--local-node <id>] [--timeout-ms <ms>] [--interface-name <name>]\n";
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

bool parseArgs(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--target-node") {
            if (i + 1 >= argc || !parseU16(argv[++i], config.targetNode)) {
                std::cerr << "Invalid or missing target node\n";
                return false;
            }
        } else if (arg == "--local-node") {
            if (i + 1 >= argc || !parseU16(argv[++i], config.localNode)) {
                std::cerr << "Invalid or missing local node\n";
                return false;
            }
        } else if (arg == "--timeout-ms") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.timeoutMs)) {
                std::cerr << "Invalid or missing timeout\n";
                return false;
            }
        } else if (arg == "--interface-name") {
            if (i + 1 >= argc) {
                return false;
            }
            config.interfaceName = argv[++i];
        } else if (arg == "--help") {
            printUsage(argv[0]);
            std::exit(0);
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return false;
        }
    }

    return config.localNode != 0U && config.targetNode != 0U;
}

const char* resultCodeName(const std::uint8_t value) {
    switch (static_cast<OBC::COMM::CSP::ResultCode>(value)) {
        case OBC::COMM::CSP::ResultCode::OK:
            return "OK";
        case OBC::COMM::CSP::ResultCode::NO_CHUNK:
            return "NO_CHUNK";
        case OBC::COMM::CSP::ResultCode::INVALID_REQUEST:
            return "INVALID_REQUEST";
        case OBC::COMM::CSP::ResultCode::IO_ERROR:
            return "IO_ERROR";
        case OBC::COMM::CSP::ResultCode::NO_CREDIT:
            return "NO_CREDIT";
        default:
            return "UNKNOWN";
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    Config config = {};
    if (!parseArgs(argc, argv, config)) {
        printUsage(argv[0]);
        return 1;
    }

    OBC::CSP::RuntimeConfig runtimeConfig =
        OBC::CSP::runtimeConfigFromEnvironment(config.localNode, config.interfaceName.c_str());
    runtimeConfig.nodeId = config.localNode;
    runtimeConfig.interfaceName = config.interfaceName;

    OBC::CSP::LibCspRuntime runtime;
    if (runtime.init(runtimeConfig) != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize CSP runtime for local node " << config.localNode << "\n";
        return 1;
    }

    OBC::COMM::CSP::DownlinkStatusV2Reply reply = {};
    std::size_t replySize = 0U;
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::TIMEOUT;

    // The ZMQ hub transport needs a brief warm-up window before the first
    // request/reply exchange from a short-lived process is reliable.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (std::uint16_t attempt = 1U; attempt <= 3U; ++attempt) {
        OBC::COMM::CSP::DownlinkStatusV2Request request = OBC::COMM::CSP::makeDownlinkStatusV2Request(attempt);
        reply = {};
        replySize = 0U;
        status = runtime.requestReply(config.targetNode,
                                      static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_STATUS_V2),
                                      &request,
                                      sizeof(request),
                                      &reply,
                                      sizeof(reply),
                                      replySize,
                                      config.timeoutMs);
        if (status == OBC::CSP::RuntimeStatus::OK) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    runtime.shutdown();

    if (status != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "DOWNLINK_STATUS_V2 request failed runtime-status=" << static_cast<int>(status) << "\n";
        return 2;
    }
    if (replySize != sizeof(reply)) {
        std::cerr << "DOWNLINK_STATUS_V2 reply size mismatch expected=" << sizeof(reply) << " actual=" << replySize
                  << "\n";
        return 3;
    }
    if (reply.header.version != OBC::COMM::CSP::VERSION_V2) {
        std::cerr << "DOWNLINK_STATUS_V2 reply version mismatch expected="
                  << static_cast<unsigned int>(OBC::COMM::CSP::VERSION_V2) << " actual="
                  << static_cast<unsigned int>(reply.header.version) << "\n";
        return 4;
    }
    if (reply.header.result != static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK)) {
        std::cerr << "DOWNLINK_STATUS_V2 rejected result=" << resultCodeName(reply.header.result) << "\n";
        return 5;
    }

    std::cout << "{"
              << "\"targetNode\":" << config.targetNode << ","
              << "\"connected\":" << (((reply.header.flags & OBC::COMM::CSP::FLAG_LINK_CONNECTED) != 0U) ? "true" : "false")
              << ","
              << "\"drainQueuedSlots\":" << reply.drainQueuedSlots << ","
              << "\"drainFreeSlots\":" << reply.drainFreeSlots << ","
              << "\"stagingActive\":" << (reply.stagingActive != 0U ? "true" : "false") << ","
              << "\"stagingStreamId\":" << reply.stagingStreamId << ","
              << "\"acceptedBytes\":" << reply.acceptedBytes << ","
              << "\"flushedBytes\":" << reply.flushedBytes << ","
              << "\"droppedCommittedBytes\":" << reply.droppedCommittedBytes << "}\n";
    return 0;
}
