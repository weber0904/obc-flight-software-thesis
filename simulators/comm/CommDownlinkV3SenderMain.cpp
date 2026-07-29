#include "simulators/comm/GroundLinkBackend.hpp"
#include "simulators/csp/CspRuntime.hpp"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr std::uint32_t kPerChunkRetrySleepMs = 100U;
constexpr std::size_t kPerChunkRetryLimit = 5U;

struct Config {
    std::uint16_t localNode = 1U;
    std::uint16_t targetNode = OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID;
    std::uint32_t waitFlushMs = 10000U;
    std::uint32_t waitFlushPollMs = 10U;
    std::size_t sendChunkBytes = OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES;
    std::string interfaceName = "DLV3SND";
    std::string sourceFile;
};

void printUsage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " --source-file <path>"
              << " [--target-node <id>]"
              << " [--local-node <id>]"
              << " [--send-chunk-bytes <bytes>]"
              << " [--wait-flush-ms <ms>]"
              << " [--wait-flush-poll-ms <ms>]"
              << " [--interface-name <name>]\n";
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

bool parseSize(const char* value, std::size_t& parsedValue) {
    char* end = nullptr;
    errno = 0;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed > std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    parsedValue = static_cast<std::size_t>(parsed);
    return true;
}

bool parseArgs(int argc, char* argv[], Config& config) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--source-file") {
            if (i + 1 >= argc) {
                return false;
            }
            config.sourceFile = argv[++i];
        } else if (arg == "--target-node") {
            if (i + 1 >= argc || !parseU16(argv[++i], config.targetNode)) {
                return false;
            }
        } else if (arg == "--local-node") {
            if (i + 1 >= argc || !parseU16(argv[++i], config.localNode)) {
                return false;
            }
        } else if (arg == "--send-chunk-bytes") {
            if (i + 1 >= argc || !parseSize(argv[++i], config.sendChunkBytes)) {
                return false;
            }
        } else if (arg == "--wait-flush-ms") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.waitFlushMs)) {
                return false;
            }
        } else if (arg == "--wait-flush-poll-ms") {
            if (i + 1 >= argc || !parseU32(argv[++i], config.waitFlushPollMs)) {
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
            return false;
        }
    }

    return !config.sourceFile.empty() && config.localNode != 0U && config.targetNode != 0U &&
           config.sendChunkBytes != 0U && config.waitFlushPollMs != 0U;
}

bool readFile(const std::string& path, std::vector<std::uint8_t>& bytesOut) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    bytesOut.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    return input.good() || input.eof();
}

const char* sendStatusName(const OBC::COMM::GroundLinkSendStatus status) {
    switch (status) {
        case OBC::COMM::GroundLinkSendStatus::OK:
            return "OK";
        case OBC::COMM::GroundLinkSendStatus::RETRY:
            return "RETRY";
        case OBC::COMM::GroundLinkSendStatus::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

std::uint32_t counterDelta(std::uint32_t current, std::uint32_t initial) {
    return static_cast<std::uint32_t>(current - initial);
}

void printStatsJson(const Config& config,
                    const OBC::COMM::GroundLinkStats& stats,
                    const OBC::COMM::GroundLinkStats& initialStats,
                    std::size_t fileSize,
                    std::size_t ackedConfirmedBytes,
                    std::size_t maxAckedBytes,
                    std::size_t maxGap,
                    std::size_t sendCalls) {
    std::cout << "{"
              << "\"targetNode\":" << config.targetNode << ","
              << "\"fileSize\":" << fileSize << ","
              << "\"sendChunkBytes\":" << config.sendChunkBytes << ","
              << "\"sendCalls\":" << sendCalls << ","
              << "\"connected\":" << (stats.connected ? "true" : "false") << ","
              << "\"txChunks\":" << stats.txChunks << ","
              << "\"txBytes\":" << stats.txBytes << ","
              << "\"txErrors\":" << stats.txErrors << ","
              << "\"txAcceptedBytes\":" << stats.txAcceptedBytes << ","
              << "\"txAcceptedBytesDelta\":" << counterDelta(stats.txAcceptedBytes, initialStats.txAcceptedBytes) << ","
              << "\"txAckedBytes\":" << stats.txAckedBytes << ","
              << "\"txFlushedBytes\":" << stats.txFlushedBytes << ","
              << "\"txFlushedBytesDelta\":" << counterDelta(stats.txFlushedBytes, initialStats.txFlushedBytes) << ","
              << "\"txDroppedBytes\":" << stats.txDroppedBytes << ","
              << "\"txQueueSlotsUsed\":" << stats.txQueueSlotsUsed << ","
              << "\"txResentFrames\":" << stats.txResentFrames << ","
              << "\"txInFlightFrames\":" << stats.txInFlightFrames << ","
              << "\"ackedConfirmedBytes\":" << ackedConfirmedBytes << ","
              << "\"maxAckedBytes\":" << maxAckedBytes << ","
              << "\"maxGapBytes\":" << maxGap << "}\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    Config config = {};
    if (!parseArgs(argc, argv, config)) {
        printUsage(argv[0]);
        return 1;
    }

    std::vector<std::uint8_t> bytes;
    if (!readFile(config.sourceFile, bytes)) {
        std::cerr << "Failed to read source file: " << config.sourceFile << "\n";
        return 2;
    }
    if (bytes.empty()) {
        std::cerr << "Source file is empty: " << config.sourceFile << "\n";
        return 3;
    }

    OBC::CSP::RuntimeConfig runtimeConfig =
        OBC::CSP::runtimeConfigFromEnvironment(config.localNode, config.interfaceName.c_str());
    runtimeConfig.nodeId = config.localNode;
    runtimeConfig.interfaceName = config.interfaceName;

    OBC::CSP::LibCspRuntime runtime;
    if (runtime.init(runtimeConfig) != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize CSP runtime for local node " << config.localNode << "\n";
        return 4;
    }

    OBC::COMM::CommCspGroundLinkBackend backend(config.targetNode, runtime);
    if (!backend.start()) {
        std::cerr << "Initial CommCspGroundLinkBackend health observation failed; continuing with send path\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        for (std::uint16_t attempt = 0U; attempt < 3U; ++attempt) {
            if (backend.observeHealth()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    std::size_t offset = 0U;
    std::size_t sendCalls = 0U;
    std::size_t ackedConfirmedBytes = 0U;
    std::size_t maxAckedBytes = 0U;
    std::size_t maxGap = 0U;
    OBC::COMM::GroundLinkStats stats = backend.getStats();
    const OBC::COMM::GroundLinkStats initialStats = stats;
    const std::uint32_t initialBacklogBytes = counterDelta(initialStats.txAcceptedBytes, initialStats.txFlushedBytes);
    const std::uint64_t requiredFlushedDelta = static_cast<std::uint64_t>(initialBacklogBytes) + bytes.size();

    while (offset < bytes.size()) {
        const std::size_t chunkBytes = std::min(config.sendChunkBytes, bytes.size() - offset);
        OBC::COMM::GroundLinkSendStatus status = OBC::COMM::GroundLinkSendStatus::ERROR;
        std::size_t attempt = 0U;
        while (attempt < kPerChunkRetryLimit) {
            status = backend.send(bytes.data() + offset, chunkBytes);
            if (status == OBC::COMM::GroundLinkSendStatus::OK) {
                break;
            }
            stats = backend.getStats();
            std::cerr << "send failed status=" << sendStatusName(status)
                      << " offset=" << offset
                      << " chunkBytes=" << chunkBytes
                      << " attempt=" << (attempt + 1U)
                      << "/" << kPerChunkRetryLimit
                      << " txErrors=" << stats.txErrors
                      << " connected=" << (stats.connected ? 1 : 0)
                      << "\n";
            if (status != OBC::COMM::GroundLinkSendStatus::RETRY || (attempt + 1U) >= kPerChunkRetryLimit) {
                backend.stop();
                runtime.shutdown();
                return status == OBC::COMM::GroundLinkSendStatus::RETRY ? 6 : 7;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(kPerChunkRetrySleepMs));
            attempt += 1U;
        }
        if (status != OBC::COMM::GroundLinkSendStatus::OK) {
            backend.stop();
            runtime.shutdown();
            return status == OBC::COMM::GroundLinkSendStatus::RETRY ? 6 : 7;
        }
        sendCalls += 1U;
        ackedConfirmedBytes += chunkBytes;
        offset += chunkBytes;
        stats = backend.getStats();
        if (stats.txAckedBytes > maxAckedBytes) {
            maxAckedBytes = stats.txAckedBytes;
        }
        const std::uint32_t acceptedDelta = counterDelta(stats.txAcceptedBytes, initialStats.txAcceptedBytes);
        const std::uint32_t flushedDelta = counterDelta(stats.txFlushedBytes, initialStats.txFlushedBytes);
        const std::size_t gap = acceptedDelta > flushedDelta ? acceptedDelta - flushedDelta : 0U;
        if (gap > maxGap) {
            maxGap = gap;
        }
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(config.waitFlushMs);
    while (counterDelta(stats.txFlushedBytes, initialStats.txFlushedBytes) < requiredFlushedDelta &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config.waitFlushPollMs));
        static_cast<void>(backend.observeHealth());
        stats = backend.getStats();
        if (stats.txAckedBytes > maxAckedBytes) {
            maxAckedBytes = stats.txAckedBytes;
        }
        const std::uint32_t acceptedDelta = counterDelta(stats.txAcceptedBytes, initialStats.txAcceptedBytes);
        const std::uint32_t flushedDelta = counterDelta(stats.txFlushedBytes, initialStats.txFlushedBytes);
        const std::size_t gap = acceptedDelta > flushedDelta ? acceptedDelta - flushedDelta : 0U;
        if (gap > maxGap) {
            maxGap = gap;
        }
    }

    stats = backend.getStats();
    printStatsJson(config, stats, initialStats, bytes.size(), ackedConfirmedBytes, maxAckedBytes, maxGap, sendCalls);

    backend.stop();
    runtime.shutdown();

    const std::uint32_t acceptedDelta = counterDelta(stats.txAcceptedBytes, initialStats.txAcceptedBytes);
    const std::uint32_t flushedDelta = counterDelta(stats.txFlushedBytes, initialStats.txFlushedBytes);
    const std::uint32_t droppedDelta = counterDelta(stats.txDroppedBytes, initialStats.txDroppedBytes);
    if (acceptedDelta < bytes.size()) {
        std::cerr << "Accepted byte count too small: expected at least " << bytes.size()
                  << " actual delta=" << acceptedDelta << "\n";
        return 8;
    }
    if (ackedConfirmedBytes < bytes.size()) {
        std::cerr << "Acked-confirmed byte count too small: expected at least " << bytes.size()
                  << " actual=" << ackedConfirmedBytes << "\n";
        return 9;
    }
    if (flushedDelta < requiredFlushedDelta) {
        std::cerr << "Flush wait timed out: expected delta at least " << requiredFlushedDelta
                  << " actual delta=" << flushedDelta << "\n";
        return 10;
    }
    if (droppedDelta != 0U) {
        std::cerr << "Dropped bytes reported by node-5 for this invocation: " << droppedDelta << "\n";
        return 11;
    }

    return 0;
}
