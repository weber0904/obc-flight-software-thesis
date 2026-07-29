#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <csp/csp.h>
#include "simulators/csp/CspRuntime.hpp"

namespace {

std::uint16_t parsePort(const char* value, std::uint16_t fallback) {
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    // Developer-only smoke binary: scripts provide validated numeric ports.
    return static_cast<std::uint16_t>(std::strtoul(value, nullptr, 10));
}

}  // namespace

int main(int argc, char* argv[]) {
    OBC::CSP::RuntimeConfig config = OBC::CSP::runtimeConfigFromEnvironment(1U, "SMOKECSP");
    std::uint16_t peerNode = 2U;
    std::uint32_t timeoutMs = 1000U;
    std::uint32_t settleMs = 200U;
    std::size_t requestReplyEchoBytes = 16U;
    std::size_t rawPayloadBytes = 16U;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--node-id" && i + 1 < argc) {
            config.nodeId = static_cast<std::uint16_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--peer-node" && i + 1 < argc) {
            peerNode = static_cast<std::uint16_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--timeout-ms" && i + 1 < argc) {
            timeoutMs = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--settle-ms" && i + 1 < argc) {
            settleMs = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--request-reply-echo-bytes" && i + 1 < argc) {
            requestReplyEchoBytes = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
        } else if (arg == "--raw-payload-bytes" && i + 1 < argc) {
            rawPayloadBytes = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
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

    if (requestReplyEchoBytes == 0U || requestReplyEchoBytes > CSP_BUFFER_SIZE || rawPayloadBytes == 0U ||
        rawPayloadBytes > CSP_BUFFER_SIZE) {
        std::cerr << "Invalid request-reply/raw payload sizing\n";
        return 1;
    }

    OBC::CSP::LibCspRuntime runtime;
    if (runtime.init(config) != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Failed to initialize local CSP runtime\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(settleMs));

    bool pingSuccess = false;
    if (runtime.ping(peerNode, timeoutMs, pingSuccess) != OBC::CSP::RuntimeStatus::OK || !pingSuccess) {
        const OBC::CSP::RuntimeMetrics metrics = runtime.metrics();
        std::cerr << "Ping to node " << peerNode << " failed"
                  << " settle_ms=" << settleMs
                  << " timeout_ms=" << timeoutMs
                  << " node=" << config.nodeId
                  << " tx=" << metrics.txPackets
                  << " rx=" << metrics.rxPackets
                  << " err=" << metrics.errorCount
                  << " free=" << metrics.freeBuffers
                  << "\n";
        runtime.shutdown();
        return 1;
    }

    if (runtime.sendRaw(peerNode, 10U, "foundation-probe") != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Raw send to node " << peerNode << " failed\n";
        runtime.shutdown();
        return 1;
    }

    auto requestReplyPing = [&](const char* phase) -> bool {
        std::vector<std::uint8_t> request(requestReplyEchoBytes);
        for (std::size_t i = 0; i < request.size(); ++i) {
            request[i] = static_cast<std::uint8_t>(i & 0xFFU);
        }
        std::vector<std::uint8_t> reply(requestReplyEchoBytes, 0U);
        std::size_t replySize = 0U;
        const OBC::CSP::RuntimeStatus status = runtime.requestReply(peerNode,
                                                                    CSP_PING,
                                                                    request.data(),
                                                                    request.size(),
                                                                    reply.data(),
                                                                    reply.size(),
                                                                    replySize,
                                                                    timeoutMs);
        if (status != OBC::CSP::RuntimeStatus::OK || replySize != request.size() ||
            std::memcmp(reply.data(), request.data(), request.size()) != 0) {
            const OBC::CSP::RuntimeMetrics metrics = runtime.metrics();
            std::cerr << "requestReply ping failed"
                      << " phase=" << phase
                      << " status=" << static_cast<int>(status)
                      << " reply_size=" << replySize
                      << " expected_reply_size=" << request.size()
                      << " node=" << config.nodeId
                      << " tx=" << metrics.txPackets
                      << " rx=" << metrics.rxPackets
                      << " err=" << metrics.errorCount
                      << " free=" << metrics.freeBuffers
                      << "\n";
            return false;
        }
        return true;
    };

    if (!requestReplyPing("before-large-sendraw")) {
        runtime.shutdown();
        return 1;
    }

    const std::string largePayload(rawPayloadBytes, 'r');
    if (runtime.sendRaw(peerNode, 10U, largePayload) != OBC::CSP::RuntimeStatus::OK) {
        std::cerr << "Large raw send to node " << peerNode << " failed"
                  << " raw_payload_bytes=" << rawPayloadBytes
                  << "\n";
        runtime.shutdown();
        return 1;
    }

    if (!requestReplyPing("after-large-sendraw")) {
        runtime.shutdown();
        return 1;
    }

    const OBC::CSP::RuntimeMetrics metrics = runtime.metrics();
    if (!metrics.initialized || metrics.localNodeId != config.nodeId || metrics.txPackets == 0U || metrics.rxPackets == 0U) {
        std::cerr << "Runtime metrics did not reflect the ping/send smoke\n";
        runtime.shutdown();
        return 1;
    }

    std::cout << "csp_runtime_smoke: node=" << metrics.localNodeId << " tx=" << metrics.txPackets
              << " rx=" << metrics.rxPackets << " free=" << metrics.freeBuffers << "\n";
    runtime.shutdown();
    return 0;
}
