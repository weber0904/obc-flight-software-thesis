#include "simulators/comm/GroundLinkBackend.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
#include <vector>

#include "simulators/comm/StreamIo.hpp"

namespace OBC {
namespace COMM {

namespace {

constexpr std::uint8_t kDownlinkV2ProbeMaxTransientAttempts = 12U;
constexpr std::uint16_t kDownlinkV3DefaultWindowFrames = 8U;
constexpr std::uint16_t kDownlinkV3SocketCanWindowFrames = 3U;

bool groundLinkDiagnosticsEnabled() {
    const char* value = std::getenv("COMM_GROUNDLINK_DIAGNOSTICS");
    return value != nullptr && std::strcmp(value, "0") != 0;
}

std::uint32_t readTimeoutOverrideOrDefault(const char* name, const std::uint32_t fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (end == value || *end != '\0' || parsed == 0UL ||
        parsed > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        return fallback;
    }
    return static_cast<std::uint32_t>(parsed);
}

std::uint16_t readBoundedU16OverrideOrDefault(const char* name,
                                              const std::uint16_t fallback,
                                              const std::uint16_t minimum,
                                              const std::uint16_t maximum) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value, &end, 10);
    if (end == value || *end != '\0' || parsed < static_cast<unsigned long>(minimum) ||
        parsed > static_cast<unsigned long>(maximum)) {
        return fallback;
    }
    return static_cast<std::uint16_t>(parsed);
}

std::string readEnvStringOrDefault(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback == nullptr ? std::string() : std::string(fallback);
    }
    return value;
}

std::string canonicalizeAsciiLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::uint16_t maxDownlinkV3WindowFramesForTransport() {
    const std::uint16_t overrideValue = readBoundedU16OverrideOrDefault(
        "COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE",
        0U,
        0U,
        static_cast<std::uint16_t>(CSP::DOWNLINK_V3_STAGING_FRAME_LIMIT));
    if (overrideValue > 0U) {
        return overrideValue;
    }
    const std::string transport = canonicalizeAsciiLower(readEnvStringOrDefault("CSP_TRANSPORT", "zmqhub"));
    if (transport == "socketcan") {
        // CFP2 packet identity only carries a 2-bit sender packet counter.
        // Keep one slot free for the following control request so bulk data
        // packets and ACK_POLL/COMMIT traffic cannot collide in reassembly.
        return kDownlinkV3SocketCanWindowFrames;
    }
    return kDownlinkV3DefaultWindowFrames;
}

std::uint16_t maxDownlinkV3DataBytesForTransport() {
    return readBoundedU16OverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES",
                                           static_cast<std::uint16_t>(CSP::DOWNLINK_V3_MAX_DATA_BYTES),
                                           1U,
                                           static_cast<std::uint16_t>(CSP::DOWNLINK_V3_MAX_DATA_BYTES));
}

std::uint32_t downlinkV3InterframeDelayUsec() {
    return readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC", 0U);
}

const char* runtimeStatusName(const OBC::CSP::RuntimeStatus status) {
    switch (status) {
        case OBC::CSP::RuntimeStatus::OK:
            return "OK";
        case OBC::CSP::RuntimeStatus::TIMEOUT:
            return "TIMEOUT";
        case OBC::CSP::RuntimeStatus::INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";
        case OBC::CSP::RuntimeStatus::EXECUTION_ERROR:
            return "EXECUTION_ERROR";
        default:
            return "UNKNOWN";
    }
}

const char* servicePortName(const CSP::ServicePort port) {
    switch (port) {
        case CSP::ServicePort::LINK_STATUS:
            return "link-status";
        case CSP::ServicePort::UPLINK_POLL:
            return "uplink-poll";
        case CSP::ServicePort::DOWNLINK_WRITE:
            return "downlink-write";
        case CSP::ServicePort::DOWNLINK_STAGE_V2:
            return "downlink-stage-v2";
        case CSP::ServicePort::DOWNLINK_STATUS_V2:
            return "downlink-status-v2";
        case CSP::ServicePort::DOWNLINK_ABORT_V2:
            return "downlink-abort-v2";
        case CSP::ServicePort::DOWNLINK_CONTROL_V3:
            return "downlink-control-v3";
        case CSP::ServicePort::DOWNLINK_DATA_V3:
            return "downlink-data-v3";
        default:
            return "unknown";
    }
}

const char* resultCodeName(const CSP::ResultCode result) {
    switch (result) {
        case CSP::ResultCode::OK:
            return "OK";
        case CSP::ResultCode::NO_CHUNK:
            return "NO_CHUNK";
        case CSP::ResultCode::INVALID_REQUEST:
            return "INVALID_REQUEST";
        case CSP::ResultCode::IO_ERROR:
            return "IO_ERROR";
        case CSP::ResultCode::NO_CREDIT:
            return "NO_CREDIT";
        case CSP::ResultCode::BUSY:
            return "BUSY";
        case CSP::ResultCode::NO_PROGRESS:
            return "NO_PROGRESS";
        default:
            return "UNKNOWN";
    }
}

const char* requestFailureReason(const OBC::CSP::RuntimeStatus status,
                                 const std::size_t replySize,
                                 const std::size_t expectedReplySize,
                                 const std::uint8_t replyVersion,
                                 const std::uint8_t expectedReplyVersion) {
    if (status != OBC::CSP::RuntimeStatus::OK) {
        switch (status) {
            case OBC::CSP::RuntimeStatus::TIMEOUT:
                return "request-timeout";
            case OBC::CSP::RuntimeStatus::INVALID_ARGUMENT:
                return "request-invalid-argument";
            case OBC::CSP::RuntimeStatus::EXECUTION_ERROR:
                return "request-execution-error";
            default:
                return "request-unknown-error";
        }
    }
    if (replySize != expectedReplySize) {
        return "reply-size-mismatch";
    }
    if (replyVersion != expectedReplyVersion) {
        return "reply-version-mismatch";
    }
    return "none";
}

void emitCommGroundLinkDiagnostic(const std::uint16_t targetNode, const std::ostringstream& line) {
    if (!groundLinkDiagnosticsEnabled()) {
        return;
    }
    std::cout << "COMM ground link diagnostic: node=" << targetNode << ' ' << line.str() << std::endl;
}

void logCommCspRequestFailure(const std::uint16_t targetNode,
                              const CSP::ServicePort port,
                              const std::uint16_t seq,
                              const OBC::CSP::RuntimeStatus status,
                              const std::size_t replySize,
                              const std::size_t expectedReplySize,
                              const std::uint8_t replyVersion,
                              const std::uint8_t expectedReplyVersion,
                              const bool connectedBefore,
                              const char* const action) {
    std::ostringstream line;
    line << "op=" << servicePortName(port) << " seq=" << seq << " runtime-status=" << runtimeStatusName(status)
         << " failure=" << requestFailureReason(status, replySize, expectedReplySize, replyVersion, expectedReplyVersion)
         << " reply-size=" << replySize << " expected-reply-size=" << expectedReplySize
         << " reply-version=" << static_cast<unsigned int>(replyVersion)
         << " expected-version=" << static_cast<unsigned int>(expectedReplyVersion)
         << " connected-before=" << (connectedBefore ? 1 : 0) << " action=" << action;
    emitCommGroundLinkDiagnostic(targetNode, line);
}

void logCommCspReplyObservation(const std::uint16_t targetNode,
                                const CSP::ServicePort port,
                                const std::uint16_t seq,
                                const CSP::ResultCode result,
                                const std::uint8_t flags,
                                const bool connectedBefore,
                                const bool connectedAfter,
                                const std::uint16_t byteCount) {
    std::ostringstream line;
    line << "op=" << servicePortName(port) << " seq=" << seq << " runtime-status=OK"
         << " result=" << resultCodeName(result) << " reply-link-connected="
         << (((flags & CSP::FLAG_LINK_CONNECTED) != 0U) ? 1 : 0) << " connected-before=" << (connectedBefore ? 1 : 0)
         << " connected-after=" << (connectedAfter ? 1 : 0) << " byte-count=" << byteCount;
    emitCommGroundLinkDiagnostic(targetNode, line);
}

void logDownlinkV2Fallback(const std::uint16_t targetNode, const char* reason) {
    std::ostringstream line;
    line << "op=downlink-status-v2 action=fallback-v1 reason=" << reason;
    emitCommGroundLinkDiagnostic(targetNode, line);
}

void logDownlinkV3Fallback(const std::uint16_t targetNode, const char* reason) {
    std::ostringstream line;
    line << "op=downlink-control-v3 action=fallback-v1 reason=" << reason;
    emitCommGroundLinkDiagnostic(targetNode, line);
}

void logDownlinkV3TransientFallback(const std::uint16_t targetNode,
                                    const std::uint8_t attempts,
                                    const std::uint32_t probeTimeoutMs,
                                    const std::uint32_t sendProbeWaitMs) {
    std::ostringstream line;
    line << "op=downlink-control-v3 action=transient-fallback-v1 attempts="
         << static_cast<unsigned int>(attempts)
         << " probe-timeout-ms=" << probeTimeoutMs
         << " send-probe-wait-ms=" << sendProbeWaitMs
         << " reason=probe-timeout";
    emitCommGroundLinkDiagnostic(targetNode, line);
}

void logDownlinkV3Phase(const std::uint16_t targetNode,
                        const char* phase,
                        const std::uint16_t streamId,
                        const std::uint16_t totalFrames,
                        const std::uint16_t contiguousFrames,
                        const std::uint32_t contiguousBytes,
                        const std::uint16_t nextFrameToSend,
                        const std::uint16_t windowSize,
                        const CSP::ResultCode result,
                        const bool connected,
                        const std::uint16_t drainQueuedFrames,
                        const std::uint16_t windowCredit) {
    std::ostringstream line;
    line << "op=downlink-control-v3 action=" << phase << " stream-id=" << streamId
         << " total-frames=" << totalFrames << " contiguous-frames=" << contiguousFrames
         << " contiguous-bytes=" << contiguousBytes << " next-frame-to-send=" << nextFrameToSend
         << " window-size=" << windowSize << " result=" << resultCodeName(result)
         << " connected=" << (connected ? 1 : 0) << " drain-queued-frames=" << drainQueuedFrames
         << " window-credit=" << windowCredit;
    emitCommGroundLinkDiagnostic(targetNode, line);
}

void logDownlinkV3DataPhase(const std::uint16_t targetNode,
                            const char* phase,
                            const std::uint16_t streamId,
                            const std::uint16_t frameIndex,
                            const std::uint16_t frameCount,
                            const std::uint16_t byteCount,
                            const std::size_t serializedSize) {
    std::ostringstream line;
    line << "op=downlink-data-v3 action=" << phase << " stream-id=" << streamId
         << " frame-index=" << frameIndex << " frame-count=" << frameCount
         << " byte-count=" << byteCount << " serialized-size=" << serializedSize;
    emitCommGroundLinkDiagnostic(targetNode, line);
}

}  // namespace

const char* groundLinkBackendModeName(GroundLinkBackendMode mode) {
    switch (mode) {
        case GroundLinkBackendMode::DIRECT_TCP:
            return "direct-tcp";
        case GroundLinkBackendMode::COMM_CSP:
            return "comm-csp";
        default:
            return "disabled";
    }
}

DirectTcpGroundLinkBackend::DirectTcpGroundLinkBackend(std::string host, std::uint16_t port, std::size_t maxChunkBytes)
    : m_host(std::move(host)),
      m_port(port),
      m_receiveBuffer(maxChunkBytes, 0U),
      m_fd(-1),
      m_stats(),
      m_successfulStatusObservations(0U) {
    this->m_stats.mode = GroundLinkBackendMode::DIRECT_TCP;
}

bool DirectTcpGroundLinkBackend::start() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->ensureConnectedLocked_();
}

void DirectTcpGroundLinkBackend::stop() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    closeFd(this->m_fd);
    this->m_stats.connected = false;
}

bool DirectTcpGroundLinkBackend::ensureConnectedLocked_() {
    if (this->m_fd >= 0) {
        this->m_stats.connected = true;
        return true;
    }

    if (!openTcpClient(this->m_host, this->m_port, this->m_fd)) {
        this->m_stats.connected = false;
        this->m_stats.txErrors += 1U;
        return false;
    }

    this->m_stats.connected = true;
    return true;
}

void DirectTcpGroundLinkBackend::handleDisconnectLocked_() {
    closeFd(this->m_fd);
    this->m_stats.connected = false;
}

GroundLinkReceiveStatus DirectTcpGroundLinkBackend::receive(std::string& outChunk, std::uint32_t timeoutMs) {
    outChunk.clear();

    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->ensureConnectedLocked_()) {
        return GroundLinkReceiveStatus::DISCONNECTED;
    }

    std::size_t bytesRead = 0U;
    const StreamReadStatus status =
        readSome(this->m_fd, timeoutMs, this->m_receiveBuffer.data(), this->m_receiveBuffer.size(), bytesRead);
    if (status == StreamReadStatus::TIMEOUT) {
        return GroundLinkReceiveStatus::IDLE;
    }
    if (status == StreamReadStatus::CLOSED) {
        this->m_stats.rxErrors += 1U;
        this->handleDisconnectLocked_();
        return GroundLinkReceiveStatus::DISCONNECTED;
    }
    if (status == StreamReadStatus::IO_ERROR) {
        this->m_stats.rxErrors += 1U;
        this->handleDisconnectLocked_();
        return GroundLinkReceiveStatus::ERROR;
    }

    outChunk.assign(reinterpret_cast<const char*>(this->m_receiveBuffer.data()), bytesRead);
    this->m_stats.rxChunks += 1U;
    this->m_stats.rxBytes += static_cast<std::uint32_t>(bytesRead);
    this->m_stats.connected = true;
    return GroundLinkReceiveStatus::DATA;
}

GroundLinkSendStatus DirectTcpGroundLinkBackend::send(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0U) {
        return GroundLinkSendStatus::ERROR;
    }

    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->ensureConnectedLocked_()) {
        return GroundLinkSendStatus::RETRY;
    }
    if (!writeAll(this->m_fd, data, size)) {
        this->m_stats.txErrors += 1U;
        this->handleDisconnectLocked_();
        return GroundLinkSendStatus::RETRY;
    }

    this->m_stats.txChunks += 1U;
    this->m_stats.txBytes += static_cast<std::uint32_t>(size);
    this->m_stats.connected = true;
    return GroundLinkSendStatus::OK;
}

GroundLinkStats DirectTcpGroundLinkBackend::getStats() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_stats;
}

GroundLinkObservationState DirectTcpGroundLinkBackend::getObservationState() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    GroundLinkObservationState observation = {};
    observation.mode = this->m_stats.mode;
    observation.healthSemantics = GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK;
    observation.connected = this->m_stats.connected;
    observation.txChunks = this->m_stats.txChunks;
    observation.rxChunks = this->m_stats.rxChunks;
    observation.txBytes = this->m_stats.txBytes;
    observation.rxBytes = this->m_stats.rxBytes;
    observation.txErrors = this->m_stats.txErrors;
    observation.rxErrors = this->m_stats.rxErrors;
    observation.successfulStatusObservations = this->m_successfulStatusObservations;
    return observation;
}

bool DirectTcpGroundLinkBackend::observeHealth() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_stats.connected = this->m_fd >= 0;
    return this->m_stats.connected;
}

CommCspGroundLinkBackend::CommCspGroundLinkBackend(std::uint16_t targetNode,
                                                   OBC::CSP::ICspRuntime& runtime,
                                                   GroundLinkHealthSemantics healthSemantics,
                                                   CommCspDownlinkPolicy downlinkPolicy)
    : m_targetNode(targetNode),
      m_runtime(runtime),
      m_healthSemantics(healthSemantics),
      m_downlinkPolicy(downlinkPolicy),
      m_mutex(),
      m_stats(),
      m_successfulStatusObservations(0U),
      m_seq(1U),
      m_streamId(1U),
      m_downlinkV3ProbeCompleted(false),
      m_downlinkV3ProbeAttempts(0U),
      m_downlinkV3Enabled(false),
      m_downlinkV3FallbackLogged(false),
      m_downlinkV2ProbeCompleted(false),
      m_downlinkV2ProbeAttempts(0U),
      m_downlinkV2Enabled(false),
      m_downlinkV2FallbackLogged(false) {
    this->m_stats.mode = GroundLinkBackendMode::COMM_CSP;
}

bool CommCspGroundLinkBackend::start() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->refreshStatusLocked_(
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_HEALTH_TIMEOUT_MS", 1000U));
}

void CommCspGroundLinkBackend::stop() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_stats.connected = false;
}

bool CommCspGroundLinkBackend::refreshStatusLocked_(std::uint32_t timeoutMs) {
    CSP::LinkStatusRequest request = CSP::makeLinkStatusRequest(this->nextSeqLocked_());
    CSP::LinkStatusReply reply = {};
    std::size_t replySize = 0U;
    const bool connectedBefore = this->m_stats.connected;
    const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
        this->m_targetNode,
        static_cast<std::uint8_t>(CSP::ServicePort::LINK_STATUS),
        &request,
        sizeof(request),
        &reply,
        sizeof(reply),
        replySize,
        timeoutMs);
    if (status != OBC::CSP::RuntimeStatus::OK || replySize != sizeof(reply) || reply.header.version != CSP::VERSION) {
        this->m_stats.rxErrors += 1U;
        logCommCspRequestFailure(this->m_targetNode,
                                 CSP::ServicePort::LINK_STATUS,
                                 request.header.seq,
                                 status,
                                 replySize,
                                 sizeof(reply),
                                 reply.header.version,
                                 CSP::VERSION,
                                 connectedBefore,
                                 "observe-health-failed");
        return false;
    }

    this->m_stats.connected = (reply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
    this->m_stats.rxChunks = reply.rxChunks;
    this->m_stats.txChunks = reply.txChunks;
    this->m_stats.rxErrors = std::max(this->m_stats.rxErrors, reply.rxErrors);
    this->m_stats.txErrors = std::max(this->m_stats.txErrors, reply.txErrors);
    this->m_successfulStatusObservations += 1U;
    if (connectedBefore != this->m_stats.connected || !this->m_stats.connected) {
        logCommCspReplyObservation(this->m_targetNode,
                                   CSP::ServicePort::LINK_STATUS,
                                   request.header.seq,
                                   static_cast<CSP::ResultCode>(reply.header.result),
                                   reply.header.flags,
                                   connectedBefore,
                                   this->m_stats.connected,
                                   0U);
    }
    if (this->m_downlinkPolicy == CommCspDownlinkPolicy::V2_ONLY && !this->m_downlinkV2ProbeCompleted) {
        static_cast<void>(this->probeDownlinkV2Locked_(timeoutMs));
    } else if (this->m_targetNode == CSP::DEFAULT_SBAND_COMM_NODE_ID && !this->m_downlinkV3ProbeCompleted) {
        static_cast<void>(this->probeDownlinkV3Locked_(timeoutMs));
    }
    if (this->m_downlinkV3Enabled) {
        static_cast<void>(this->refreshDownlinkV3StatusLocked_(timeoutMs));
    } else if (this->m_downlinkV2Enabled) {
        static_cast<void>(this->refreshDownlinkV2StatusLocked_(timeoutMs));
    }
    return true;
}

std::uint16_t CommCspGroundLinkBackend::nextSeqLocked_() {
    const std::uint16_t current = this->m_seq;
    this->m_seq = static_cast<std::uint16_t>(this->m_seq + 1U);
    if (this->m_seq == 0U) {
        this->m_seq = 1U;
    }
    return current;
}

std::uint16_t CommCspGroundLinkBackend::nextStreamIdLocked_() {
    const std::uint16_t current = this->m_streamId;
    this->m_streamId = static_cast<std::uint16_t>(this->m_streamId + 1U);
    if (this->m_streamId == 0U) {
        this->m_streamId = 1U;
    }
    return current;
}

bool CommCspGroundLinkBackend::probeDownlinkV3Locked_(std::uint32_t timeoutMs) {
    if (this->m_downlinkV3ProbeCompleted) {
        return this->m_downlinkV3Enabled;
    }
    if (this->m_targetNode != CSP::DEFAULT_SBAND_COMM_NODE_ID) {
        this->m_downlinkV3ProbeCompleted = true;
        return false;
    }

    CSP::DownlinkControlV3Request request =
        CSP::makeDownlinkControlV3Request(CSP::DownlinkControlV3Op::STATUS, this->nextSeqLocked_(), 0U);
    CSP::DownlinkControlV3Reply reply = {};
    std::size_t replySize = 0U;
    const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
        this->m_targetNode,
        static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_CONTROL_V3),
        &request,
        sizeof(request),
        &reply,
        sizeof(reply),
        replySize,
        timeoutMs);
    const bool validReply = status == OBC::CSP::RuntimeStatus::OK && replySize == sizeof(reply) &&
                            reply.header.version == CSP::VERSION_V3 &&
                            reply.header.result == static_cast<std::uint8_t>(CSP::ResultCode::OK) &&
                            reply.op == static_cast<std::uint8_t>(CSP::DownlinkControlV3Op::STATUS);
    if (!validReply) {
        this->m_downlinkV3ProbeAttempts = static_cast<std::uint8_t>(this->m_downlinkV3ProbeAttempts + 1U);
        const bool definitiveFailure = status == OBC::CSP::RuntimeStatus::OK;
        if (definitiveFailure && !this->m_downlinkV3FallbackLogged) {
            this->m_downlinkV3FallbackLogged = true;
            logDownlinkV3Fallback(this->m_targetNode,
                                  requestFailureReason(
                                      status, replySize, sizeof(reply), reply.header.version, CSP::VERSION_V3));
        }
        if (definitiveFailure) {
            this->m_downlinkV3ProbeCompleted = true;
        }
        return false;
    }

    this->m_downlinkV3ProbeCompleted = true;
    this->m_downlinkV3Enabled = true;
    this->m_downlinkV3ProbeAttempts = 0U;
    this->updateDownlinkV3StatsLocked_(reply);
    return true;
}

bool CommCspGroundLinkBackend::ensureDownlinkV3ReadyForSendLocked_() {
    if (this->m_downlinkV3Enabled || this->m_downlinkV3ProbeCompleted ||
        this->m_targetNode != CSP::DEFAULT_SBAND_COMM_NODE_ID) {
        return this->m_downlinkV3Enabled;
    }

    const std::uint32_t probeTimeoutMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_PROBE_TIMEOUT_MS", 1000U);
    const std::uint32_t sendProbeWaitMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_WAIT_MS", probeTimeoutMs);
    const std::uint32_t retrySleepMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_RETRY_SLEEP_MS", 20U);
    const std::uint8_t attemptsBefore = this->m_downlinkV3ProbeAttempts;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(sendProbeWaitMs);

    do {
        if (this->probeDownlinkV3Locked_(probeTimeoutMs)) {
            return true;
        }
        if (this->m_downlinkV3ProbeCompleted) {
            return false;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            break;
        }
        if (retrySleepMs > 0U) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retrySleepMs));
        }
    } while (std::chrono::steady_clock::now() < deadline);

    if (groundLinkDiagnosticsEnabled()) {
        const std::uint8_t attemptsUsed =
            static_cast<std::uint8_t>(this->m_downlinkV3ProbeAttempts - attemptsBefore);
        logDownlinkV3TransientFallback(this->m_targetNode, attemptsUsed, probeTimeoutMs, sendProbeWaitMs);
    }
    return false;
}

bool CommCspGroundLinkBackend::refreshDownlinkV3StatusLocked_(std::uint32_t timeoutMs) {
    if (!this->m_downlinkV3Enabled) {
        return false;
    }

    CSP::DownlinkControlV3Request request =
        CSP::makeDownlinkControlV3Request(CSP::DownlinkControlV3Op::STATUS, this->nextSeqLocked_(), 0U);
    CSP::DownlinkControlV3Reply reply = {};
    std::size_t replySize = 0U;
    const bool connectedBefore = this->m_stats.connected;
    const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
        this->m_targetNode,
        static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_CONTROL_V3),
        &request,
        sizeof(request),
        &reply,
        sizeof(reply),
        replySize,
        timeoutMs);
    if (status != OBC::CSP::RuntimeStatus::OK || replySize != sizeof(reply) || reply.header.version != CSP::VERSION_V3) {
        logCommCspRequestFailure(this->m_targetNode,
                                 CSP::ServicePort::DOWNLINK_CONTROL_V3,
                                 request.header.seq,
                                 status,
                                 replySize,
                                 sizeof(reply),
                                 reply.header.version,
                                 CSP::VERSION_V3,
                                 connectedBefore,
                                 "observe-health-failed");
        return false;
    }

    this->m_stats.connected = (reply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
    this->updateDownlinkV3StatsLocked_(reply);
    return reply.header.result == static_cast<std::uint8_t>(CSP::ResultCode::OK);
}

bool CommCspGroundLinkBackend::probeDownlinkV2Locked_(std::uint32_t timeoutMs) {
    if (this->m_downlinkV2ProbeCompleted) {
        return this->m_downlinkV2Enabled;
    }
    if (this->m_targetNode != CSP::DEFAULT_SBAND_COMM_NODE_ID) {
        this->m_downlinkV2ProbeCompleted = true;
        return false;
    }

    CSP::DownlinkStatusV2Request request = CSP::makeDownlinkStatusV2Request(this->nextSeqLocked_());
    CSP::DownlinkStatusV2Reply reply = {};
    std::size_t replySize = 0U;
    const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
        this->m_targetNode,
        static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_STATUS_V2),
        &request,
        sizeof(request),
        &reply,
        sizeof(reply),
        replySize,
        timeoutMs);
    const bool validReply = status == OBC::CSP::RuntimeStatus::OK && replySize == sizeof(reply) &&
                            reply.header.version == CSP::VERSION_V2 &&
                            reply.header.result == static_cast<std::uint8_t>(CSP::ResultCode::OK);
    if (!validReply) {
        this->m_downlinkV2ProbeAttempts = static_cast<std::uint8_t>(this->m_downlinkV2ProbeAttempts + 1U);
        const bool definitiveFailure = status == OBC::CSP::RuntimeStatus::OK;
        const bool giveUp = definitiveFailure || this->m_downlinkV2ProbeAttempts >= kDownlinkV2ProbeMaxTransientAttempts;
        if (giveUp && !this->m_downlinkV2FallbackLogged) {
            this->m_downlinkV2FallbackLogged = true;
            logDownlinkV2Fallback(this->m_targetNode, requestFailureReason(status,
                                                                           replySize,
                                                                           sizeof(reply),
                                                                           reply.header.version,
                                                                           CSP::VERSION_V2));
        }
        if (giveUp) {
            this->m_downlinkV2ProbeCompleted = true;
        }
        return false;
    }

    this->m_downlinkV2ProbeCompleted = true;
    this->m_downlinkV2Enabled = true;
    this->updateDownlinkV2StatsLocked_(reply);
    return true;
}

bool CommCspGroundLinkBackend::refreshDownlinkV2StatusLocked_(std::uint32_t timeoutMs) {
    if (!this->m_downlinkV2Enabled) {
        return false;
    }

    CSP::DownlinkStatusV2Request request = CSP::makeDownlinkStatusV2Request(this->nextSeqLocked_());
    CSP::DownlinkStatusV2Reply reply = {};
    std::size_t replySize = 0U;
    const bool connectedBefore = this->m_stats.connected;
    const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
        this->m_targetNode,
        static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_STATUS_V2),
        &request,
        sizeof(request),
        &reply,
        sizeof(reply),
        replySize,
        timeoutMs);
    if (status != OBC::CSP::RuntimeStatus::OK || replySize != sizeof(reply) || reply.header.version != CSP::VERSION_V2) {
        logCommCspRequestFailure(this->m_targetNode,
                                 CSP::ServicePort::DOWNLINK_STATUS_V2,
                                 request.header.seq,
                                 status,
                                 replySize,
                                 sizeof(reply),
                                 reply.header.version,
                                 CSP::VERSION_V2,
                                 connectedBefore,
                                 "observe-health-failed");
        return false;
    }

    this->m_stats.connected = (reply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
    this->updateDownlinkV2StatsLocked_(reply);
    return reply.header.result == static_cast<std::uint8_t>(CSP::ResultCode::OK);
}

void CommCspGroundLinkBackend::updateDownlinkV2StatsLocked_(const CSP::DownlinkStatusV2Reply& reply) {
    this->m_stats.txAcceptedBytes = reply.acceptedBytes;
    this->m_stats.txFlushedBytes = reply.flushedBytes;
    this->m_stats.txDroppedBytes = reply.droppedCommittedBytes;
    this->m_stats.txQueueSlotsUsed = reply.drainQueuedSlots;
}

void CommCspGroundLinkBackend::updateDownlinkV3StatsLocked_(const CSP::DownlinkControlV3Reply& reply) {
    this->m_stats.txAcceptedBytes = reply.acceptedBytes;
    this->m_stats.txFlushedBytes = reply.flushedBytes;
    this->m_stats.txDroppedBytes = reply.droppedCommittedBytes;
    this->m_stats.txAckedBytes = reply.contiguousBytes;
    this->m_stats.txQueueSlotsUsed = reply.drainQueuedFrames;
}

bool CommCspGroundLinkBackend::abortDownlinkV3Locked_(std::uint16_t streamId) {
    if (!this->m_downlinkV3Enabled) {
        return false;
    }

    CSP::DownlinkControlV3Request request =
        CSP::makeDownlinkControlV3Request(CSP::DownlinkControlV3Op::ABORT, this->nextSeqLocked_(), streamId);
    CSP::DownlinkControlV3Reply reply = {};
    std::size_t replySize = 0U;
    const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
        this->m_targetNode,
        static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_CONTROL_V3),
        &request,
        sizeof(request),
        &reply,
        sizeof(reply),
        replySize,
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_CONTROL_TIMEOUT_MS", 1000U));
    return status == OBC::CSP::RuntimeStatus::OK && replySize == sizeof(reply) && reply.header.version == CSP::VERSION_V3;
}

bool CommCspGroundLinkBackend::abortDownlinkV2Locked_(std::uint16_t streamId) {
    if (!this->m_downlinkV2Enabled) {
        return false;
    }

    CSP::DownlinkAbortV2Request request = CSP::makeDownlinkAbortV2Request(this->nextSeqLocked_(), streamId);
    CSP::DownlinkAbortV2Reply reply = {};
    std::size_t replySize = 0U;
    const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
        this->m_targetNode,
        static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_ABORT_V2),
        &request,
        sizeof(request),
        &reply,
        sizeof(reply),
        replySize,
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V2_STAGE_TIMEOUT_MS", 1000U));
    return status == OBC::CSP::RuntimeStatus::OK && replySize == sizeof(reply) && reply.header.version == CSP::VERSION_V2;
}

GroundLinkReceiveStatus CommCspGroundLinkBackend::receive(std::string& outChunk, std::uint32_t timeoutMs) {
    outChunk.clear();

    std::lock_guard<std::mutex> lock(this->m_mutex);
    CSP::UplinkPollRequest request = CSP::makeUplinkPollRequest(this->nextSeqLocked_());
    CSP::ChunkReply reply = {};
    std::size_t replySize = 0U;
    const bool connectedBefore = this->m_stats.connected;
    const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
        this->m_targetNode,
        static_cast<std::uint8_t>(CSP::ServicePort::UPLINK_POLL),
        &request,
        sizeof(request),
        &reply,
        sizeof(reply),
        replySize,
        timeoutMs);
    if (status != OBC::CSP::RuntimeStatus::OK || replySize != sizeof(reply) || reply.header.version != CSP::VERSION) {
        // Generic node 4 remains a compatibility path. Preserve its hosted
        // idle behavior; active health semantics are assigned in observation.
        if (status == OBC::CSP::RuntimeStatus::TIMEOUT && this->m_targetNode == CSP::DEFAULT_GENERIC_COMM_NODE_ID) {
            return GroundLinkReceiveStatus::IDLE;
        }
        this->m_stats.connected = false;
        this->m_stats.rxErrors += 1U;
        logCommCspRequestFailure(this->m_targetNode,
                                 CSP::ServicePort::UPLINK_POLL,
                                 request.header.seq,
                                 status,
                                 replySize,
                                 sizeof(reply),
                                 reply.header.version,
                                 CSP::VERSION,
                                 connectedBefore,
                                 "mark-disconnected");
        return GroundLinkReceiveStatus::DISCONNECTED;
    }

    this->m_successfulStatusObservations += 1U;
    this->m_stats.connected = (reply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
    const CSP::ResultCode result = static_cast<CSP::ResultCode>(reply.header.result);
    if (connectedBefore != this->m_stats.connected || !this->m_stats.connected || result != CSP::ResultCode::NO_CHUNK) {
        logCommCspReplyObservation(this->m_targetNode,
                                   CSP::ServicePort::UPLINK_POLL,
                                   request.header.seq,
                                   result,
                                   reply.header.flags,
                                   connectedBefore,
                                   this->m_stats.connected,
                                   reply.header.byteCount);
    }
    if (result == CSP::ResultCode::NO_CHUNK) {
        return GroundLinkReceiveStatus::IDLE;
    }
    if (result != CSP::ResultCode::OK || reply.header.byteCount > CSP::MAX_CHUNK_BYTES) {
        this->m_stats.rxErrors += 1U;
        return GroundLinkReceiveStatus::ERROR;
    }

    outChunk.assign(reinterpret_cast<const char*>(reply.data), reply.header.byteCount);
    this->m_stats.rxChunks += 1U;
    this->m_stats.rxBytes += reply.header.byteCount;
    return GroundLinkReceiveStatus::DATA;
}

GroundLinkSendStatus CommCspGroundLinkBackend::send(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr || size == 0U) {
        return GroundLinkSendStatus::ERROR;
    }

    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (this->m_downlinkPolicy == CommCspDownlinkPolicy::V2_ONLY) {
        if (!this->m_downlinkV2ProbeCompleted) {
            static_cast<void>(this->probeDownlinkV2Locked_(
                readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V2_STAGE_TIMEOUT_MS", 1000U)));
        }
        return this->m_downlinkV2Enabled ? this->sendV2Locked_(data, size) : GroundLinkSendStatus::RETRY;
    }
    if (this->m_targetNode == CSP::DEFAULT_SBAND_COMM_NODE_ID && !this->m_downlinkV3ProbeCompleted) {
        static_cast<void>(this->ensureDownlinkV3ReadyForSendLocked_());
    }
    return this->m_downlinkV3Enabled ? this->sendV3Locked_(data, size) : this->sendV1Locked_(data, size);
}

GroundLinkSendStatus CommCspGroundLinkBackend::sendV1Locked_(const std::uint8_t* data, std::size_t size) {
    std::size_t offset = 0U;
    while (offset < size) {
        const std::size_t chunkSize = std::min(size - offset, static_cast<std::size_t>(CSP::MAX_CHUNK_BYTES));

        CSP::DownlinkWriteRequest request = CSP::makeDownlinkWriteRequest(this->nextSeqLocked_());
        request.byteCount = static_cast<std::uint16_t>(chunkSize);
        std::memcpy(request.data, data + offset, chunkSize);

        CSP::ChunkReply reply = {};
        std::size_t replySize = 0U;
        const bool connectedBefore = this->m_stats.connected;
        const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
            this->m_targetNode,
            static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_WRITE),
            &request,
            sizeof(request),
            &reply,
            sizeof(reply),
            replySize,
            readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS", 1000U));
        if (status != OBC::CSP::RuntimeStatus::OK || replySize != sizeof(reply) || reply.header.version != CSP::VERSION) {
            this->m_stats.connected = false;
            this->m_stats.txErrors += 1U;
            logCommCspRequestFailure(this->m_targetNode,
                                     CSP::ServicePort::DOWNLINK_WRITE,
                                     request.header.seq,
                                     status,
                                     replySize,
                                     sizeof(reply),
                                     reply.header.version,
                                     CSP::VERSION,
                                     connectedBefore,
                                     "mark-disconnected");
            return GroundLinkSendStatus::RETRY;
        }

        this->m_stats.connected = (reply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
        const CSP::ResultCode result = static_cast<CSP::ResultCode>(reply.header.result);
        if (connectedBefore != this->m_stats.connected || !this->m_stats.connected || result != CSP::ResultCode::OK) {
            logCommCspReplyObservation(this->m_targetNode,
                                       CSP::ServicePort::DOWNLINK_WRITE,
                                       request.header.seq,
                                       result,
                                       reply.header.flags,
                                       connectedBefore,
                                       this->m_stats.connected,
                                       reply.header.byteCount);
        }
        if (result != CSP::ResultCode::OK) {
            this->m_stats.txErrors += 1U;
            return this->m_stats.connected ? GroundLinkSendStatus::ERROR : GroundLinkSendStatus::RETRY;
        }

        this->m_stats.txChunks += 1U;
        this->m_stats.txBytes += static_cast<std::uint32_t>(chunkSize);
        offset += chunkSize;
    }

    return GroundLinkSendStatus::OK;
}

GroundLinkSendStatus CommCspGroundLinkBackend::sendV3Locked_(const std::uint8_t* data, std::size_t size) {
    const std::uint16_t streamId = this->nextStreamIdLocked_();
    const std::uint32_t controlTimeoutMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_CONTROL_TIMEOUT_MS", 1000U);
    const std::uint32_t commitWaitMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_COMMIT_WAIT_MS", 2000U);
    const std::uint32_t retrySleepMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_RETRY_SLEEP_MS", 20U);
    const std::uint32_t controlTurnaroundMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V3_CONTROL_TURNAROUND_MS", 0U);
    const std::uint32_t interframeDelayUsec = downlinkV3InterframeDelayUsec();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(commitWaitMs);
    const std::uint16_t dataBytesPerFrame = maxDownlinkV3DataBytesForTransport();
    const std::uint16_t totalFrames = static_cast<std::uint16_t>((size + dataBytesPerFrame - 1U) / dataBytesPerFrame);
    if (totalFrames == 0U || totalFrames > CSP::DOWNLINK_V3_STAGING_FRAME_LIMIT) {
        this->m_stats.txErrors += 1U;
        return GroundLinkSendStatus::ERROR;
    }

    auto requestControl = [&](CSP::DownlinkControlV3Op op,
                              std::uint16_t committedFrames,
                              std::uint32_t committedBytes,
                              CSP::DownlinkControlV3Reply& outReply) -> OBC::CSP::RuntimeStatus {
        std::size_t replySize = 0U;
        CSP::DownlinkControlV3Request request = CSP::makeDownlinkControlV3Request(op, this->nextSeqLocked_(), streamId);
        request.totalFrames = totalFrames;
        request.totalBytes = static_cast<std::uint32_t>(size);
        request.committedFrames = committedFrames;
        request.committedBytes = committedBytes;
        const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
            this->m_targetNode,
            static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_CONTROL_V3),
            &request,
            sizeof(request),
            &outReply,
            sizeof(outReply),
            replySize,
            controlTimeoutMs);
        if (status != OBC::CSP::RuntimeStatus::OK) {
            if (groundLinkDiagnosticsEnabled()) {
                const OBC::CSP::RuntimeMetrics metrics = this->m_runtime.metrics();
                std::ostringstream line;
                line << "op=downlink-control-v3 action=request-runtime-failed control-op="
                     << static_cast<unsigned int>(op) << " stream-id=" << streamId
                     << " runtime-status=" << runtimeStatusName(status)
                     << " reply-size=" << replySize
                     << " expected-reply-size=" << sizeof(outReply)
                     << " reply-version=" << static_cast<unsigned int>(outReply.header.version)
                     << " free-buffers=" << metrics.freeBuffers
                     << " runtime-errors=" << metrics.errorCount
                     << " tx-packets=" << metrics.txPackets
                     << " rx-packets=" << metrics.rxPackets;
                emitCommGroundLinkDiagnostic(this->m_targetNode, line);
            }
            return status;
        }
        if (replySize != sizeof(outReply) || outReply.header.version != CSP::VERSION_V3) {
            if (groundLinkDiagnosticsEnabled()) {
                const OBC::CSP::RuntimeMetrics metrics = this->m_runtime.metrics();
                std::ostringstream line;
                line << "op=downlink-control-v3 action=request-shape-failed control-op="
                     << static_cast<unsigned int>(op) << " stream-id=" << streamId
                     << " reply-size=" << replySize
                     << " expected-reply-size=" << sizeof(outReply)
                     << " reply-version=" << static_cast<unsigned int>(outReply.header.version)
                     << " expected-version=" << static_cast<unsigned int>(CSP::VERSION_V3)
                     << " free-buffers=" << metrics.freeBuffers
                     << " runtime-errors=" << metrics.errorCount
                     << " tx-packets=" << metrics.txPackets
                     << " rx-packets=" << metrics.rxPackets;
                emitCommGroundLinkDiagnostic(this->m_targetNode, line);
            }
            return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
        }
        return OBC::CSP::RuntimeStatus::OK;
    };

    bool streamActiveOnNode = false;
    std::uint16_t contiguousFrames = 0U;
    std::uint32_t contiguousBytes = 0U;
    std::uint16_t nextFrameToSend = 0U;
    const std::uint16_t windowCap =
        static_cast<std::uint16_t>(std::min<std::size_t>(maxDownlinkV3WindowFramesForTransport(),
                                                         CSP::DOWNLINK_V3_STAGING_FRAME_LIMIT));
    std::uint16_t windowSize = static_cast<std::uint16_t>(std::min<std::size_t>(totalFrames, windowCap));

    CSP::DownlinkControlV3Reply beginReply = {};
    OBC::CSP::RuntimeStatus beginStatus = OBC::CSP::RuntimeStatus::TIMEOUT;
    do {
        beginReply = {};
        beginStatus = requestControl(CSP::DownlinkControlV3Op::BEGIN, 0U, 0U, beginReply);
        if (beginStatus != OBC::CSP::RuntimeStatus::TIMEOUT || std::chrono::steady_clock::now() >= deadline) {
            break;
        }
        if (retrySleepMs > 0U) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retrySleepMs));
        }
    } while (std::chrono::steady_clock::now() < deadline);
    if (beginStatus != OBC::CSP::RuntimeStatus::OK ||
        beginReply.header.result != static_cast<std::uint8_t>(CSP::ResultCode::OK)) {
        logDownlinkV3Phase(this->m_targetNode,
                           "begin-failed",
                           streamId,
                           totalFrames,
                           contiguousFrames,
                           contiguousBytes,
                           nextFrameToSend,
                           windowSize,
                           static_cast<CSP::ResultCode>(beginReply.header.result),
                           this->m_stats.connected,
                           beginReply.drainQueuedFrames,
                           beginReply.windowCredit);
        this->m_stats.txErrors += 1U;
        return GroundLinkSendStatus::RETRY;
    }
    streamActiveOnNode = true;
    this->m_stats.connected = (beginReply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
    this->updateDownlinkV3StatsLocked_(beginReply);
    logDownlinkV3Phase(this->m_targetNode,
                       "begin-ok",
                       streamId,
                       totalFrames,
                       contiguousFrames,
                       contiguousBytes,
                       nextFrameToSend,
                       windowSize,
                       static_cast<CSP::ResultCode>(beginReply.header.result),
                       this->m_stats.connected,
                       beginReply.drainQueuedFrames,
                       beginReply.windowCredit);

    while (contiguousFrames < totalFrames) {
        while (nextFrameToSend < totalFrames && nextFrameToSend < static_cast<std::uint16_t>(contiguousFrames + windowSize)) {
            const std::size_t byteOffset = static_cast<std::size_t>(nextFrameToSend) * dataBytesPerFrame;
            const std::size_t chunkSize = std::min(size - byteOffset, static_cast<std::size_t>(dataBytesPerFrame));
            CSP::DownlinkDataV3Frame frame = CSP::makeDownlinkDataV3Frame(streamId);
            frame.frameIndex = nextFrameToSend;
            frame.frameCount = totalFrames;
            frame.byteOffset = static_cast<std::uint32_t>(byteOffset);
            frame.byteCount = static_cast<std::uint16_t>(chunkSize);
            std::memcpy(frame.data, data + byteOffset, chunkSize);
            std::string payload;
            if (!CSP::serializeDownlinkDataV3Frame(frame, payload)) {
                logDownlinkV3DataPhase(this->m_targetNode,
                                       "serialize-failed",
                                       streamId,
                                       frame.frameIndex,
                                       frame.frameCount,
                                       frame.byteCount,
                                       0U);
                this->m_stats.txErrors += 1U;
                static_cast<void>(this->abortDownlinkV3Locked_(streamId));
                return GroundLinkSendStatus::ERROR;
            }
            if (this->m_runtime.sendRaw(this->m_targetNode,
                                        static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_DATA_V3),
                                        payload) != OBC::CSP::RuntimeStatus::OK) {
                logDownlinkV3DataPhase(this->m_targetNode,
                                       "send-failed",
                                       streamId,
                                       frame.frameIndex,
                                       frame.frameCount,
                                       frame.byteCount,
                                       payload.size());
                this->m_stats.txErrors += 1U;
                static_cast<void>(this->abortDownlinkV3Locked_(streamId));
                return GroundLinkSendStatus::RETRY;
            }
            logDownlinkV3DataPhase(this->m_targetNode,
                                   "send-ok",
                                   streamId,
                                   frame.frameIndex,
                                   frame.frameCount,
                                   frame.byteCount,
                                   payload.size());
            this->m_stats.txChunks += 1U;
            this->m_stats.txBytes += static_cast<std::uint32_t>(chunkSize);
            this->m_stats.txInFlightFrames = (nextFrameToSend + 1U) - contiguousFrames;
            nextFrameToSend = static_cast<std::uint16_t>(nextFrameToSend + 1U);
            if (interframeDelayUsec > 0U && nextFrameToSend < totalFrames) {
                std::this_thread::sleep_for(std::chrono::microseconds(interframeDelayUsec));
            }
        }

        CSP::DownlinkControlV3Reply ackReply = {};
        if (requestControl(CSP::DownlinkControlV3Op::ACK_POLL, contiguousFrames, contiguousBytes, ackReply) !=
            OBC::CSP::RuntimeStatus::OK) {
            logDownlinkV3Phase(this->m_targetNode,
                               "ack-failed",
                               streamId,
                               totalFrames,
                               contiguousFrames,
                               contiguousBytes,
                               nextFrameToSend,
                               windowSize,
                               static_cast<CSP::ResultCode>(ackReply.header.result),
                               this->m_stats.connected,
                               ackReply.drainQueuedFrames,
                               ackReply.windowCredit);
            if (std::chrono::steady_clock::now() >= deadline) {
                this->m_stats.txErrors += 1U;
                static_cast<void>(this->abortDownlinkV3Locked_(streamId));
                return GroundLinkSendStatus::RETRY;
            }
            const std::uint16_t resendEnd = nextFrameToSend;
            if (resendEnd > contiguousFrames) {
                this->m_stats.txResentFrames += resendEnd - contiguousFrames;
            }
            nextFrameToSend = contiguousFrames;
            this->m_stats.txInFlightFrames =
                resendEnd > contiguousFrames ? static_cast<std::uint16_t>(resendEnd - contiguousFrames) : 0U;
            std::this_thread::sleep_for(std::chrono::milliseconds(retrySleepMs));
            continue;
        }
        this->m_stats.connected = (ackReply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
        this->updateDownlinkV3StatsLocked_(ackReply);
        logDownlinkV3Phase(this->m_targetNode,
                           "ack-reply",
                           streamId,
                           totalFrames,
                           ackReply.contiguousFrames,
                           ackReply.contiguousBytes,
                           nextFrameToSend,
                           windowSize,
                           static_cast<CSP::ResultCode>(ackReply.header.result),
                           this->m_stats.connected,
                           ackReply.drainQueuedFrames,
                           ackReply.windowCredit);
        const std::uint16_t previousContiguousFrames = contiguousFrames;
        const std::uint32_t previousContiguousBytes = contiguousBytes;
        contiguousFrames = std::max(contiguousFrames, ackReply.contiguousFrames);
        contiguousBytes = std::max(contiguousBytes, ackReply.contiguousBytes);
        this->m_stats.txInFlightFrames = nextFrameToSend > contiguousFrames ? nextFrameToSend - contiguousFrames : 0U;
        if (ackReply.windowCredit > 0U) {
            windowSize = static_cast<std::uint16_t>(std::min<std::uint16_t>(ackReply.windowCredit, windowCap));
        }

        if (contiguousFrames == totalFrames) {
            break;
        }

        const CSP::ResultCode ackResult = static_cast<CSP::ResultCode>(ackReply.header.result);
        const bool madeProgress =
            contiguousFrames > previousContiguousFrames || contiguousBytes > previousContiguousBytes;
        if (!madeProgress || ackResult == CSP::ResultCode::NO_PROGRESS) {
            if (std::chrono::steady_clock::now() >= deadline) {
                this->m_stats.txErrors += 1U;
                static_cast<void>(this->abortDownlinkV3Locked_(streamId));
                return GroundLinkSendStatus::RETRY;
            }
            const std::uint16_t resendEnd = nextFrameToSend;
            if (resendEnd > contiguousFrames) {
                this->m_stats.txResentFrames += resendEnd - contiguousFrames;
            }
            nextFrameToSend = contiguousFrames;
            std::this_thread::sleep_for(std::chrono::milliseconds(retrySleepMs));
        }
    }

    if (controlTurnaroundMs > 0U) {
        std::this_thread::sleep_for(std::chrono::milliseconds(controlTurnaroundMs));
    }

    while (true) {
        CSP::DownlinkControlV3Reply commitReply = {};
        if (requestControl(CSP::DownlinkControlV3Op::COMMIT, contiguousFrames, contiguousBytes, commitReply) !=
            OBC::CSP::RuntimeStatus::OK) {
            logDownlinkV3Phase(this->m_targetNode,
                               "commit-failed",
                               streamId,
                               totalFrames,
                               contiguousFrames,
                               contiguousBytes,
                               nextFrameToSend,
                               windowSize,
                               static_cast<CSP::ResultCode>(commitReply.header.result),
                               this->m_stats.connected,
                               commitReply.drainQueuedFrames,
                               commitReply.windowCredit);
            if (std::chrono::steady_clock::now() >= deadline) {
                this->m_stats.txErrors += 1U;
                static_cast<void>(this->abortDownlinkV3Locked_(streamId));
                return GroundLinkSendStatus::RETRY;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(retrySleepMs));
            continue;
        }
        this->m_stats.connected = (commitReply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
        this->updateDownlinkV3StatsLocked_(commitReply);
        this->m_stats.txInFlightFrames = 0U;
        const CSP::ResultCode result = static_cast<CSP::ResultCode>(commitReply.header.result);
        logDownlinkV3Phase(this->m_targetNode,
                           "commit-reply",
                           streamId,
                           totalFrames,
                           commitReply.contiguousFrames,
                           commitReply.contiguousBytes,
                           nextFrameToSend,
                           windowSize,
                           result,
                           this->m_stats.connected,
                           commitReply.drainQueuedFrames,
                           commitReply.windowCredit);
        if (result == CSP::ResultCode::OK) {
            streamActiveOnNode = false;
            return GroundLinkSendStatus::OK;
        }
        if (result == CSP::ResultCode::NO_CREDIT && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retrySleepMs));
            continue;
        }
        this->m_stats.txErrors += 1U;
        if (streamActiveOnNode) {
            static_cast<void>(this->abortDownlinkV3Locked_(streamId));
        }
        return GroundLinkSendStatus::RETRY;
    }
}

GroundLinkSendStatus CommCspGroundLinkBackend::sendV2Locked_(const std::uint8_t* data, std::size_t size) {
    const std::uint16_t streamId = this->nextStreamIdLocked_();
    const std::uint32_t stageTimeoutMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V2_STAGE_TIMEOUT_MS", 1000U);
    const std::uint32_t commitWaitMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V2_COMMIT_WAIT_MS", 2000U);
    const std::uint32_t retrySleepMs =
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_DOWNLINK_V2_RETRY_SLEEP_MS", 20U);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(commitWaitMs);

    std::size_t offset = 0U;
    std::uint16_t chunkSeq = 1U;
    bool streamActiveOnNode = false;

    while (offset < size) {
        const std::size_t chunkSize =
            std::min(size - offset, static_cast<std::size_t>(CSP::DOWNLINK_STAGE_V2_MAX_DATA_BYTES));
        const bool first = offset == 0U;
        const bool last = (offset + chunkSize) == size;

        CSP::DownlinkStageV2Request request = CSP::makeDownlinkStageV2Request(chunkSeq, streamId);
        request.byteCount = static_cast<std::uint16_t>(chunkSize);
        request.flags = static_cast<std::uint8_t>((first ? CSP::FLAG_DOWNLINK_STAGE_V2_FIRST : 0U) |
                                                  (last ? CSP::FLAG_DOWNLINK_STAGE_V2_LAST : 0U));
        std::memcpy(request.data, data + offset, chunkSize);

        while (true) {
            CSP::DownlinkStageV2Reply reply = {};
            std::size_t replySize = 0U;
            const bool connectedBefore = this->m_stats.connected;
            const OBC::CSP::RuntimeStatus status = this->m_runtime.requestReply(
                this->m_targetNode,
                static_cast<std::uint8_t>(CSP::ServicePort::DOWNLINK_STAGE_V2),
                &request,
                sizeof(request),
                &reply,
                sizeof(reply),
                replySize,
                stageTimeoutMs);
            if (status != OBC::CSP::RuntimeStatus::OK || replySize != sizeof(reply) || reply.header.version != CSP::VERSION_V2) {
                this->m_stats.connected = false;
                this->m_stats.txErrors += 1U;
                logCommCspRequestFailure(this->m_targetNode,
                                         CSP::ServicePort::DOWNLINK_STAGE_V2,
                                         request.header.seq,
                                         status,
                                         replySize,
                                         sizeof(reply),
                                         reply.header.version,
                                         CSP::VERSION_V2,
                                         connectedBefore,
                                         "mark-disconnected");
                if (streamActiveOnNode) {
                    static_cast<void>(this->abortDownlinkV2Locked_(streamId));
                }
                return GroundLinkSendStatus::RETRY;
            }

            this->m_stats.connected = (reply.header.flags & CSP::FLAG_LINK_CONNECTED) != 0U;
            const CSP::ResultCode result = static_cast<CSP::ResultCode>(reply.header.result);
            if (connectedBefore != this->m_stats.connected || !this->m_stats.connected || result != CSP::ResultCode::OK) {
                logCommCspReplyObservation(this->m_targetNode,
                                           CSP::ServicePort::DOWNLINK_STAGE_V2,
                                           request.header.seq,
                                           result,
                                           reply.header.flags,
                                           connectedBefore,
                                           this->m_stats.connected,
                                           request.byteCount);
            }

            if (result == CSP::ResultCode::OK) {
                this->m_stats.txChunks += 1U;
                this->m_stats.txBytes += static_cast<std::uint32_t>(chunkSize);
                this->m_stats.txQueueSlotsUsed =
                    static_cast<std::uint32_t>(CSP::DOWNLINK_STAGE_V2_DRAIN_SLOTS - reply.drainFreeSlots);
                streamActiveOnNode = !last;
                if (last) {
                    static_cast<void>(this->refreshDownlinkV2StatusLocked_(stageTimeoutMs));
                }
                offset += chunkSize;
                chunkSeq = static_cast<std::uint16_t>(chunkSeq + 1U);
                break;
            }

            if (result == CSP::ResultCode::NO_CREDIT && last) {
                streamActiveOnNode = true;
                this->m_stats.txQueueSlotsUsed =
                    static_cast<std::uint32_t>(CSP::DOWNLINK_STAGE_V2_DRAIN_SLOTS - reply.drainFreeSlots);
                if (std::chrono::steady_clock::now() >= deadline) {
                    this->m_stats.txErrors += 1U;
                    static_cast<void>(this->abortDownlinkV2Locked_(streamId));
                    return GroundLinkSendStatus::RETRY;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(retrySleepMs));
                continue;
            }

            this->m_stats.txErrors += 1U;
            if (streamActiveOnNode) {
                static_cast<void>(this->abortDownlinkV2Locked_(streamId));
            }
            return this->m_stats.connected ? GroundLinkSendStatus::ERROR : GroundLinkSendStatus::RETRY;
        }
    }

    return GroundLinkSendStatus::OK;
}

GroundLinkStats CommCspGroundLinkBackend::getStats() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_stats;
}

GroundLinkObservationState CommCspGroundLinkBackend::getObservationState() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    GroundLinkObservationState observation = {};
    observation.mode = this->m_stats.mode;
    observation.healthSemantics = this->m_healthSemantics;
    observation.connected = this->m_stats.connected;
    observation.txChunks = this->m_stats.txChunks;
    observation.rxChunks = this->m_stats.rxChunks;
    observation.txBytes = this->m_stats.txBytes;
    observation.rxBytes = this->m_stats.rxBytes;
    observation.txErrors = this->m_stats.txErrors;
    observation.rxErrors = this->m_stats.rxErrors;
    observation.successfulStatusObservations = this->m_successfulStatusObservations;
    return observation;
}

bool CommCspGroundLinkBackend::observeHealth() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->refreshStatusLocked_(
        readTimeoutOverrideOrDefault("COMM_GROUNDLINK_HEALTH_TIMEOUT_MS", 1000U));
}

std::unique_ptr<IGroundLinkBackend> makeDirectTcpGroundLinkBackend(const std::string& host,
                                                                   std::uint16_t port,
                                                                   std::size_t maxChunkBytes) {
    return std::unique_ptr<IGroundLinkBackend>(new DirectTcpGroundLinkBackend(host, port, maxChunkBytes));
}

std::unique_ptr<IGroundLinkBackend> makeCommCspGroundLinkBackend(std::uint16_t targetNode,
                                                                 OBC::CSP::ICspRuntime& runtime,
                                                                 GroundLinkHealthSemantics healthSemantics) {
    return std::unique_ptr<IGroundLinkBackend>(new CommCspGroundLinkBackend(targetNode, runtime, healthSemantics));
}

}  // namespace COMM
}  // namespace OBC
