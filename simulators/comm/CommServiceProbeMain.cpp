#include "simulators/comm/CommCspProtocol.hpp"
#include "simulators/comm/StreamIo.hpp"
#include "simulators/csp/CspRuntime.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace {

constexpr const char* kDownlinkPayloadPrefix = "downlink-";
constexpr const char* kUplinkPayloadPrefix = "uplink-";

struct Config {
    std::uint16_t localNode = 7U;
    std::uint16_t targetNode = OBC::COMM::CSP::DEFAULT_COMM_NODE_ID;
    std::string linkIdentity = "generic";
    std::string serialPeer;
    std::uint32_t baudrate = 115200U;
    std::uint32_t timeoutMs = 1000U;
    std::string interfaceName = "COMMPRB";
};

void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " --target-node <id> --serial-peer <path> [--local-node <id>]"
              << " [--link <name>] [--baudrate <rate>] [--timeout-ms <ms>] [--interface-name <name>]\n";
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
        } else if (arg == "--link") {
            if (i + 1 >= argc) {
                return false;
            }
            config.linkIdentity = argv[++i];
        } else if (arg == "--serial-peer") {
            if (i + 1 >= argc) {
                return false;
            }
            config.serialPeer = argv[++i];
        } else if (arg == "--baudrate") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.baudrate)) {
                std::cerr << "Invalid or missing baudrate\n";
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
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return false;
        }
    }

    if (config.targetNode == 0U || config.localNode == 0U || config.serialPeer.empty()) {
        return false;
    }
    const std::size_t maxLinkIdentityLength =
        OBC::COMM::CSP::MAX_CHUNK_BYTES - std::strlen(kDownlinkPayloadPrefix);
    if (config.linkIdentity.size() > maxLinkIdentityLength) {
        std::cerr << "Link identity is too long for COMM service probe payloads; max length is "
                  << maxLinkIdentityLength << " bytes\n";
        return false;
    }
    return true;
}

bool readExact(int fd, std::string& out, std::size_t size, std::uint32_t timeoutMs) {
    out.clear();
    out.reserve(size);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (out.size() < size && std::chrono::steady_clock::now() < deadline) {
        std::uint8_t buffer[256] = {};
        std::size_t bytesRead = 0U;
        const std::size_t remaining = size - out.size();
        const OBC::COMM::StreamReadStatus status =
            OBC::COMM::readSome(fd, 100U, buffer, std::min<std::size_t>(sizeof(buffer), remaining), bytesRead);
        if (status == OBC::COMM::StreamReadStatus::TIMEOUT) {
            continue;
        }
        if (status != OBC::COMM::StreamReadStatus::OK || bytesRead == 0U) {
            return false;
        }
        out.append(reinterpret_cast<const char*>(buffer), bytesRead);
    }

    return out.size() == size;
}

bool requestReply(OBC::CSP::ICspRuntime& runtime,
                  std::uint16_t targetNode,
                  OBC::COMM::CSP::ServicePort port,
                  const void* request,
                  std::size_t requestSize,
                  void* reply,
                  std::size_t replyCapacity,
                  std::size_t& replySize,
                  std::uint32_t timeoutMs) {
    return runtime.requestReply(targetNode,
                                static_cast<std::uint8_t>(port),
                                request,
                                requestSize,
                                reply,
                                replyCapacity,
                                replySize,
                                timeoutMs) == OBC::CSP::RuntimeStatus::OK;
}

bool linkStatus(OBC::CSP::ICspRuntime& runtime,
                const Config& config,
                OBC::COMM::CSP::LinkStatusReply& reply,
                std::uint16_t& seq) {
    const OBC::COMM::CSP::LinkStatusRequest request = OBC::COMM::CSP::makeLinkStatusRequest(seq++);
    std::size_t replySize = 0U;
    if (!requestReply(runtime,
                      config.targetNode,
                      OBC::COMM::CSP::ServicePort::LINK_STATUS,
                      &request,
                      sizeof(request),
                      &reply,
                      sizeof(reply),
                      replySize,
                      config.timeoutMs)) {
        std::cerr << "LINK_STATUS request failed for node " << config.targetNode << "\n";
        return false;
    }
    const bool valid = replySize == sizeof(reply) && reply.header.version == OBC::COMM::CSP::VERSION &&
                       reply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK);
    if (!valid) {
        std::cerr << "LINK_STATUS returned invalid reply for node " << config.targetNode
                  << " replySize=" << replySize
                  << " version=" << static_cast<unsigned int>(reply.header.version)
                  << " result=" << static_cast<unsigned int>(reply.header.result) << "\n";
    }
    return valid;
}

bool waitForPing(OBC::CSP::ICspRuntime& runtime, const Config& config) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(config.timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        bool success = false;
        if (runtime.ping(config.targetNode, 250U, success) == OBC::CSP::RuntimeStatus::OK && success) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cerr << "CSP ping failed from local node " << config.localNode
              << " to target node " << config.targetNode << "\n";
    return false;
}

bool probeDownlink(OBC::CSP::ICspRuntime& runtime, const Config& config, int peerFd, std::uint16_t& seq) {
    const std::string payload = std::string(kDownlinkPayloadPrefix) + config.linkIdentity;
    OBC::COMM::CSP::DownlinkWriteRequest request = OBC::COMM::CSP::makeDownlinkWriteRequest(seq++);
    request.byteCount = static_cast<std::uint16_t>(payload.size());
    std::memcpy(request.data, payload.data(), payload.size());

    OBC::COMM::CSP::ChunkReply reply = {};
    std::size_t replySize = 0U;
    if (!requestReply(runtime,
                      config.targetNode,
                      OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE,
                      &request,
                      sizeof(request),
                      &reply,
                      sizeof(reply),
                      replySize,
                      config.timeoutMs)) {
        std::cerr << "DOWNLINK_WRITE request failed for node " << config.targetNode << "\n";
        return false;
    }
    if (replySize != sizeof(reply) || reply.header.result != static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK)) {
        std::cerr << "DOWNLINK_WRITE rejected for node " << config.targetNode << "\n";
        return false;
    }

    std::string observed;
    if (!readExact(peerFd, observed, payload.size(), config.timeoutMs) || observed != payload) {
        std::cerr << "Serial peer did not observe expected downlink payload for node " << config.targetNode << "\n";
        return false;
    }
    return true;
}

bool probeUplink(OBC::CSP::ICspRuntime& runtime, const Config& config, int peerFd, std::uint16_t& seq) {
    const std::string payload = std::string(kUplinkPayloadPrefix) + config.linkIdentity;
    if (!OBC::COMM::writeAll(peerFd, reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size())) {
        std::cerr << "Failed to write uplink payload to serial peer for node " << config.targetNode << "\n";
        return false;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(config.timeoutMs);
    while (std::chrono::steady_clock::now() < deadline) {
        OBC::COMM::CSP::UplinkPollRequest request = OBC::COMM::CSP::makeUplinkPollRequest(seq++);
        OBC::COMM::CSP::ChunkReply reply = {};
        std::size_t replySize = 0U;
        if (!requestReply(runtime,
                          config.targetNode,
                          OBC::COMM::CSP::ServicePort::UPLINK_POLL,
                          &request,
                          sizeof(request),
                          &reply,
                          sizeof(reply),
                          replySize,
                          250U)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (replySize != sizeof(reply) || reply.header.version != OBC::COMM::CSP::VERSION) {
            return false;
        }
        const OBC::COMM::CSP::ResultCode result = static_cast<OBC::COMM::CSP::ResultCode>(reply.header.result);
        if (result == OBC::COMM::CSP::ResultCode::NO_CHUNK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (result != OBC::COMM::CSP::ResultCode::OK || reply.header.byteCount != payload.size()) {
            return false;
        }
        const std::string observed(reinterpret_cast<const char*>(reply.data), reply.header.byteCount);
        return observed == payload;
    }

    std::cerr << "UPLINK_POLL did not return expected payload for node " << config.targetNode << "\n";
    return false;
}

}  // namespace

int main(int argc, char* argv[]) {
    Config config = {};
    if (!parseArgs(argc, argv, config)) {
        printUsage(argv[0]);
        return 2;
    }

    int peerFd = -1;
    if (!OBC::COMM::openRawSerial(config.serialPeer, config.baudrate, peerFd)) {
        std::cerr << "Failed to open serial peer " << config.serialPeer << "\n";
        return 1;
    }

    OBC::CSP::RuntimeConfig runtimeConfig =
        OBC::CSP::runtimeConfigFromEnvironment(config.localNode, config.interfaceName.c_str());
    OBC::CSP::LibCspRuntime runtime;
    if (runtime.init(runtimeConfig) != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize probe CSP runtime for local node " << config.localNode << "\n";
        OBC::COMM::closeFd(peerFd);
        return 1;
    }
    if (!waitForPing(runtime, config)) {
        runtime.shutdown();
        OBC::COMM::closeFd(peerFd);
        return 1;
    }

    std::uint16_t seq = 1U;
    OBC::COMM::CSP::LinkStatusReply status = {};
    const bool ok = linkStatus(runtime, config, status, seq) &&
                    probeDownlink(runtime, config, peerFd, seq) &&
                    probeUplink(runtime, config, peerFd, seq) &&
                    linkStatus(runtime, config, status, seq);

    const OBC::CSP::RuntimeMetrics metrics = runtime.metrics();
    runtime.shutdown();
    OBC::COMM::closeFd(peerFd);

    if (!ok) {
        return 1;
    }

    std::cout << "comm_service_probe: PASS"
              << " link=" << config.linkIdentity
              << " targetNode=" << config.targetNode
              << " localNode=" << config.localNode
              << " rxChunks=" << status.rxChunks
              << " txChunks=" << status.txChunks
              << " rxErrors=" << status.rxErrors
              << " txErrors=" << status.txErrors
              << " probeTx=" << metrics.txPackets
              << " probeRx=" << metrics.rxPackets << "\n";
    return 0;
}
