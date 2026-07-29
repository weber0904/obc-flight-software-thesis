#include "simulators/comm/CommCspProtocol.hpp"
#include "simulators/comm/GroundGatewayProxy.hpp"
#include "simulators/comm/GroundLinkBackend.hpp"
#include "simulators/comm/CommNodeServer.hpp"
#include "simulators/comm/StreamIo.hpp"
#include "simulators/csp/CspRuntime.hpp"

#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <functional>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

class FakeCspRuntime final : public OBC::CSP::ICspRuntime {
  public:
    using RequestReplyHandler = std::function<OBC::CSP::RuntimeStatus(std::uint16_t,
                                                                      std::uint8_t,
                                                                      const void*,
                                                                      std::size_t,
                                                                      void*,
                                                                      std::size_t,
                                                                      std::size_t&,
                                                                      std::uint32_t)>;
    using SendRawHandler =
        std::function<OBC::CSP::RuntimeStatus(std::uint16_t, std::uint8_t, const std::string&)>;

    RequestReplyHandler requestReplyHandler;
    SendRawHandler sendRawHandler;

    OBC::CSP::RuntimeStatus init(const OBC::CSP::RuntimeConfig&) override {
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus ping(std::uint16_t, std::uint32_t, bool& success) override {
        success = true;
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus sendRaw(std::uint16_t targetNode, std::uint8_t targetPort, const std::string& payload) override {
        if (this->sendRawHandler) {
            return this->sendRawHandler(targetNode, targetPort, payload);
        }
        return OBC::CSP::RuntimeStatus::OK;
    }

    OBC::CSP::RuntimeStatus requestReply(std::uint16_t targetNode,
                                         std::uint8_t targetPort,
                                         const void* requestData,
                                         std::size_t requestSize,
                                         void* replyData,
                                         std::size_t replyCapacity,
                                         std::size_t& replySize,
                                         std::uint32_t timeoutMs) override {
        assert(this->requestReplyHandler);
        return this->requestReplyHandler(
            targetNode, targetPort, requestData, requestSize, replyData, replyCapacity, replySize, timeoutMs);
    }

    OBC::CSP::RuntimeMetrics metrics() const override {
        return OBC::CSP::RuntimeMetrics{};
    }

    void shutdown() override {}
};

class ScopedEnvVar final {
  public:
    ScopedEnvVar(const char* name, const char* value) : m_name(name == nullptr ? "" : name) {
        const char* existing = std::getenv(this->m_name.c_str());
        if (existing != nullptr) {
            this->m_hadOriginal = true;
            this->m_originalValue = existing;
        }
        const int rc = ::setenv(this->m_name.c_str(), value == nullptr ? "" : value, 1);
        assert(rc == 0);
    }

    ~ScopedEnvVar() {
        if (this->m_hadOriginal) {
            const int rc = ::setenv(this->m_name.c_str(), this->m_originalValue.c_str(), 1);
            assert(rc == 0);
            return;
        }
        const int rc = ::unsetenv(this->m_name.c_str());
        assert(rc == 0);
    }

  private:
    std::string m_name;
    std::string m_originalValue;
    bool m_hadOriginal = false;
};

bool writeAll(int fd, const std::uint8_t* data, std::size_t size) {
    std::size_t offset = 0U;
    while (offset < size) {
        const ssize_t written = ::write(fd, data + offset, size - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool readExact(int fd, std::string& out, std::size_t size, int timeoutMs = 3000) {
    out.clear();
    out.reserve(size);

    while (out.size() < size) {
        pollfd pfd = {};
        pfd.fd = fd;
        pfd.events = POLLIN;
        const int pollStatus = ::poll(&pfd, 1, timeoutMs);
        if (pollStatus <= 0) {
            return false;
        }

        char buffer[256] = {};
        const std::size_t chunkSize = std::min(size - out.size(), sizeof(buffer));
        const ssize_t bytesRead = ::read(fd, buffer, chunkSize);
        if (bytesRead <= 0) {
            if (bytesRead < 0 && errno == EINTR) {
                continue;
            }
            return false;
        }
        out.append(buffer, static_cast<std::size_t>(bytesRead));
    }

    return true;
}

std::string readFile(const std::string& path) {
    std::ifstream stream(path, std::ios::binary);
    assert(stream.good());
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

OBC::COMM::CSP::DownlinkStageV2Request makeDownlinkStageRequest(std::uint16_t streamId,
                                                                std::uint16_t seq,
                                                                const std::string& payload,
                                                                bool first,
                                                                bool last) {
    OBC::COMM::CSP::DownlinkStageV2Request request = OBC::COMM::CSP::makeDownlinkStageV2Request(seq, streamId);
    request.byteCount = static_cast<std::uint16_t>(payload.size());
    request.flags = static_cast<std::uint8_t>((first ? OBC::COMM::CSP::FLAG_DOWNLINK_STAGE_V2_FIRST : 0U) |
                                              (last ? OBC::COMM::CSP::FLAG_DOWNLINK_STAGE_V2_LAST : 0U));
    std::memcpy(request.data, payload.data(), payload.size());
    return request;
}

OBC::COMM::CSP::DownlinkControlV3Reply makeDownlinkV3Reply(std::uint16_t seq,
                                                           OBC::COMM::CSP::DownlinkControlV3Op op,
                                                           OBC::COMM::CSP::ResultCode result) {
    OBC::COMM::CSP::DownlinkControlV3Reply reply = OBC::COMM::CSP::makeDownlinkControlV3Reply(op, seq, result);
    reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
    reply.drainFreeFrames = static_cast<std::uint16_t>(OBC::COMM::CSP::DOWNLINK_V3_DRAIN_FRAME_LIMIT);
    reply.windowCredit = static_cast<std::uint16_t>(OBC::COMM::CSP::DOWNLINK_V3_STAGING_FRAME_LIMIT);
    return reply;
}

OBC::COMM::CSP::DownlinkDataV3Frame decodeDownlinkV3Frame(const std::string& payload) {
    OBC::COMM::CSP::DownlinkDataV3Frame frame = {};
    assert(OBC::COMM::CSP::deserializeDownlinkDataV3Frame(payload.data(), payload.size(), frame));
    return frame;
}

class TcpServer {
  public:
    TcpServer() : m_listenFd(-1), m_clientFd(-1), m_port(0U) {
        this->m_listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(this->m_listenFd >= 0);

        int opt = 1;
        (void)::setsockopt(this->m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address = {};
#ifdef __APPLE__
        address.sin_len = sizeof(address);
#endif
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(0U);
        assert(::bind(this->m_listenFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
        assert(::listen(this->m_listenFd, 1) == 0);

        socklen_t addressLength = sizeof(address);
        assert(::getsockname(this->m_listenFd, reinterpret_cast<sockaddr*>(&address), &addressLength) == 0);
        this->m_port = ntohs(address.sin_port);
    }

    ~TcpServer() {
        if (this->m_clientFd >= 0) {
            ::close(this->m_clientFd);
            this->m_clientFd = -1;
        }
        if (this->m_listenFd >= 0) {
            ::close(this->m_listenFd);
            this->m_listenFd = -1;
        }
    }

    std::uint16_t port() const {
        return this->m_port;
    }

    int acceptClient(int timeoutMs = 3000) {
        pollfd pfd = {};
        pfd.fd = this->m_listenFd;
        pfd.events = POLLIN;
        const int pollStatus = ::poll(&pfd, 1, timeoutMs);
        assert(pollStatus > 0);
        this->m_clientFd = ::accept(this->m_listenFd, nullptr, nullptr);
        assert(this->m_clientFd >= 0);
        return this->m_clientFd;
    }

    void closeClientWithReset() {
        assert(this->m_clientFd >= 0);
        linger resetOnClose = {};
        resetOnClose.l_onoff = 1;
        resetOnClose.l_linger = 0;
        (void)::setsockopt(this->m_clientFd, SOL_SOCKET, SO_LINGER, &resetOnClose, sizeof(resetOnClose));
        ::close(this->m_clientFd);
        this->m_clientFd = -1;
    }

  private:
    int m_listenFd;
    int m_clientFd;
    std::uint16_t m_port;
};

class PtyPeer {
  public:
    PtyPeer() : m_masterFd(-1), m_slavePath() {
        this->m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
        assert(this->m_masterFd >= 0);
        assert(::grantpt(this->m_masterFd) == 0);
        assert(::unlockpt(this->m_masterFd) == 0);
        char* const slave = ::ptsname(this->m_masterFd);
        assert(slave != nullptr);
        this->m_slavePath = slave;
    }

    ~PtyPeer() {
        if (this->m_masterFd >= 0) {
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
        }
    }

    int masterFd() const {
        return this->m_masterFd;
    }

    const std::string& slavePath() const {
        return this->m_slavePath;
    }

  private:
    int m_masterFd;
    std::string m_slavePath;
};

void testCommCspReceiveData() {
    FakeCspRuntime runtime;
    runtime.requestReplyHandler =
        [](std::uint16_t targetNode,
           std::uint8_t targetPort,
           const void* requestData,
           std::size_t requestSize,
           void* replyData,
           std::size_t replyCapacity,
           std::size_t& replySize,
           std::uint32_t timeoutMs) {
            assert(targetNode == OBC::COMM::CSP::DEFAULT_COMM_NODE_ID);
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::UPLINK_POLL));
            assert(requestSize == sizeof(OBC::COMM::CSP::UplinkPollRequest));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::ChunkReply));
            assert(timeoutMs == 25U);

            const auto* request = static_cast<const OBC::COMM::CSP::UplinkPollRequest*>(requestData);
            assert(request->header.version == OBC::COMM::CSP::VERSION);
            assert(request->maxBytes == OBC::COMM::CSP::MAX_CHUNK_BYTES);

            OBC::COMM::CSP::ChunkReply reply = OBC::COMM::CSP::makeChunkReply(
                OBC::COMM::CSP::ServicePort::UPLINK_POLL, request->header.seq, OBC::COMM::CSP::ResultCode::OK);
            reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
            reply.header.byteCount = 3U;
            reply.data[0] = 'A';
            reply.data[1] = 'B';
            reply.data[2] = 'C';
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_COMM_NODE_ID, runtime);
    std::string chunk;
    const OBC::COMM::GroundLinkReceiveStatus status = backend.receive(chunk, 25U);
    assert(status == OBC::COMM::GroundLinkReceiveStatus::DATA);
    assert(chunk == "ABC");

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(stats.connected);
    assert(stats.rxChunks == 1U);
    assert(stats.rxBytes == 3U);
}

void testCommCspSendSplitsLargePayload() {
    FakeCspRuntime runtime;
    std::size_t requestCount = 0U;
    runtime.requestReplyHandler =
        [&requestCount](std::uint16_t,
                        std::uint8_t targetPort,
                        const void* requestData,
                        std::size_t requestSize,
                        void* replyData,
                        std::size_t replyCapacity,
                        std::size_t& replySize,
                        std::uint32_t timeoutMs) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE));
            assert(requestSize == sizeof(OBC::COMM::CSP::DownlinkWriteRequest));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::ChunkReply));
            assert(timeoutMs == 1000U);

            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkWriteRequest*>(requestData);
            if (requestCount == 0U) {
                assert(request->byteCount == OBC::COMM::CSP::MAX_CHUNK_BYTES);
                assert(request->data[0] == 'a');
                assert(request->data[OBC::COMM::CSP::MAX_CHUNK_BYTES - 1U] == 'a');
            } else {
                assert(request->byteCount == 5U);
                assert(request->data[0] == 'a');
            }

            OBC::COMM::CSP::ChunkReply reply = OBC::COMM::CSP::makeChunkReply(
                OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE, request->header.seq, OBC::COMM::CSP::ResultCode::OK);
            reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            requestCount += 1U;
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_COMM_NODE_ID, runtime);
    const std::string payload(OBC::COMM::CSP::MAX_CHUNK_BYTES + 5U, 'a');
    const OBC::COMM::GroundLinkSendStatus status =
        backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size());
    assert(status == OBC::COMM::GroundLinkSendStatus::OK);
    assert(requestCount == 2U);

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(stats.connected);
    assert(stats.txChunks == 2U);
    assert(stats.txBytes == payload.size());
}

void testCommCspStartRefreshesStatus() {
    FakeCspRuntime runtime;
    runtime.requestReplyHandler =
        [](std::uint16_t,
           std::uint8_t targetPort,
           const void* requestData,
           std::size_t requestSize,
           void* replyData,
           std::size_t replyCapacity,
           std::size_t& replySize,
           std::uint32_t timeoutMs) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::LINK_STATUS));
            assert(requestSize == sizeof(OBC::COMM::CSP::LinkStatusRequest));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::LinkStatusReply));
            assert(timeoutMs == 1000U);

            const auto* request = static_cast<const OBC::COMM::CSP::LinkStatusRequest*>(requestData);
            OBC::COMM::CSP::LinkStatusReply reply =
                OBC::COMM::CSP::makeLinkStatusReply(request->header.seq, OBC::COMM::CSP::ResultCode::OK);
            reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
            reply.rxChunks = 2U;
            reply.txChunks = 3U;
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_COMM_NODE_ID, runtime);
    assert(backend.start());

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(stats.connected);
    assert(stats.rxChunks == 2U);
    assert(stats.txChunks == 3U);
}

void testCommCspIdlePollRefreshesObservationFreshness() {
    FakeCspRuntime runtime;
    runtime.requestReplyHandler =
        [](std::uint16_t,
           std::uint8_t targetPort,
           const void* requestData,
           std::size_t requestSize,
           void* replyData,
           std::size_t replyCapacity,
           std::size_t& replySize,
           std::uint32_t timeoutMs) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::UPLINK_POLL));
            assert(requestSize == sizeof(OBC::COMM::CSP::UplinkPollRequest));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::ChunkReply));
            assert(timeoutMs == 25U);

            const auto* request = static_cast<const OBC::COMM::CSP::UplinkPollRequest*>(requestData);
            OBC::COMM::CSP::ChunkReply reply = OBC::COMM::CSP::makeChunkReply(
                OBC::COMM::CSP::ServicePort::UPLINK_POLL, request->header.seq, OBC::COMM::CSP::ResultCode::NO_CHUNK);
            reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_COMM_NODE_ID, runtime);
    std::string chunk;
    const OBC::COMM::GroundLinkReceiveStatus status = backend.receive(chunk, 25U);
    assert(status == OBC::COMM::GroundLinkReceiveStatus::IDLE);
    assert(chunk.empty());

    const OBC::COMM::GroundLinkObservationState observation = backend.getObservationState();
    assert(observation.connected);
    assert(observation.successfulStatusObservations == 1U);
}

void testActiveCommNodeTimeoutMarksDisconnected() {
    FakeCspRuntime runtime;
    std::size_t requestCount = 0U;
    runtime.requestReplyHandler =
        [&requestCount](std::uint16_t targetNode,
                        std::uint8_t targetPort,
                        const void* requestData,
                        std::size_t requestSize,
                        void* replyData,
                        std::size_t replyCapacity,
                        std::size_t& replySize,
                        std::uint32_t timeoutMs) {
            assert(targetNode == OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID);
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::UPLINK_POLL));
            assert(requestSize == sizeof(OBC::COMM::CSP::UplinkPollRequest));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::ChunkReply));
            assert(timeoutMs == 25U);

            if (requestCount == 0U) {
                const auto* request = static_cast<const OBC::COMM::CSP::UplinkPollRequest*>(requestData);
                OBC::COMM::CSP::ChunkReply reply =
                    OBC::COMM::CSP::makeChunkReply(
                        OBC::COMM::CSP::ServicePort::UPLINK_POLL, request->header.seq, OBC::COMM::CSP::ResultCode::NO_CHUNK);
                reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
                std::memcpy(replyData, &reply, sizeof(reply));
                replySize = sizeof(reply);
                requestCount += 1U;
                return OBC::CSP::RuntimeStatus::OK;
            }

            requestCount += 1U;
            return OBC::CSP::RuntimeStatus::TIMEOUT;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID, runtime);
    std::string chunk;
    assert(backend.receive(chunk, 25U) == OBC::COMM::GroundLinkReceiveStatus::IDLE);
    assert(backend.receive(chunk, 25U) == OBC::COMM::GroundLinkReceiveStatus::DISCONNECTED);

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(!stats.connected);
    assert(stats.rxErrors == 1U);
    assert(requestCount == 2U);
}

void testCommCspSendTimeoutRequestsRetryAndClearsConnected() {
    FakeCspRuntime runtime;
    std::size_t requestCount = 0U;
    runtime.requestReplyHandler =
        [&requestCount](std::uint16_t targetNode,
                        std::uint8_t targetPort,
                        const void* requestData,
                        std::size_t requestSize,
                        void* replyData,
                        std::size_t replyCapacity,
                        std::size_t& replySize,
                        std::uint32_t timeoutMs) {
            assert(targetNode == OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID);
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE));
            assert(requestSize == sizeof(OBC::COMM::CSP::DownlinkWriteRequest));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::ChunkReply));
            assert(timeoutMs == 1000U);

            if (requestCount == 0U) {
                const auto* request = static_cast<const OBC::COMM::CSP::DownlinkWriteRequest*>(requestData);
                OBC::COMM::CSP::ChunkReply reply = OBC::COMM::CSP::makeChunkReply(
                    OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE, request->header.seq, OBC::COMM::CSP::ResultCode::OK);
                reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
                assert(request->byteCount == 4U);
                std::memcpy(replyData, &reply, sizeof(reply));
                replySize = sizeof(reply);
                requestCount += 1U;
                return OBC::CSP::RuntimeStatus::OK;
            }

            requestCount += 1U;
            return OBC::CSP::RuntimeStatus::TIMEOUT;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID, runtime);
    const std::string payload("PING");
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::RETRY);

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(!stats.connected);
    assert(stats.txChunks == 1U);
    assert(stats.txErrors == 1U);
    assert(requestCount == 2U);
}

void testCommCspObservationPublishesConfiguredSemantics() {
    FakeCspRuntime runtime;
    runtime.requestReplyHandler =
        [](std::uint16_t,
           std::uint8_t,
           const void*,
           std::size_t,
           void*,
           std::size_t,
           std::size_t&,
           std::uint32_t) { return OBC::CSP::RuntimeStatus::TIMEOUT; };

    OBC::COMM::CommCspGroundLinkBackend genericBackend(
        OBC::COMM::CSP::DEFAULT_GENERIC_COMM_NODE_ID,
        runtime,
        OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK);
    OBC::COMM::CommCspGroundLinkBackend sbandBackend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                                     runtime,
                                                     OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP);
    OBC::COMM::CommCspGroundLinkBackend uhfBackend(OBC::COMM::CSP::DEFAULT_UHF_COMM_NODE_ID,
                                                   runtime,
                                                   OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP);
    OBC::COMM::CommCspGroundLinkBackend sbandDisabledBackend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                                             runtime,
                                                             OBC::COMM::GroundLinkHealthSemantics::DISABLED);

    assert(genericBackend.getObservationState().healthSemantics ==
           OBC::COMM::GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK);
    assert(sbandBackend.getObservationState().healthSemantics == OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP);
    assert(uhfBackend.getObservationState().healthSemantics == OBC::COMM::GroundLinkHealthSemantics::ACTIVE_COMM_CSP);
    assert(sbandDisabledBackend.getObservationState().healthSemantics == OBC::COMM::GroundLinkHealthSemantics::DISABLED);
}

void testCommCspNode5ProbeSelectsV3AndSendUsesControlPlusRawData() {
    FakeCspRuntime runtime;
    std::vector<OBC::COMM::CSP::DownlinkControlV3Op> controlOps;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    const std::string payload(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES + 17U, 'v');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t targetNode, std::uint8_t targetPort, const std::string& raw) {
        assert(targetNode == OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID);
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_DATA_V3));
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&controlOps, &payload](std::uint16_t targetNode,
                                std::uint8_t targetPort,
                                const void* requestData,
                                std::size_t requestSize,
                                void* replyData,
                                std::size_t replyCapacity,
                                std::size_t& replySize,
                                std::uint32_t) {
            assert(targetNode == OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID);
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            assert(requestSize == sizeof(OBC::COMM::CSP::DownlinkControlV3Request));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::DownlinkControlV3Reply));

            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            controlOps.push_back(op);

            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            reply.streamId = request->streamId;
            reply.totalFrames = request->totalFrames;
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL) {
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
                reply.windowCredit = 0U;
            } else if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
                reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
                reply.drainQueuedFrames = request->totalFrames;
                reply.drainFreeFrames = static_cast<std::uint16_t>(OBC::COMM::CSP::DOWNLINK_V3_DRAIN_FRAME_LIMIT -
                                                                   request->totalFrames);
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);

    assert(controlOps.size() == 4U);
    assert(controlOps[0] == OBC::COMM::CSP::DownlinkControlV3Op::STATUS);
    assert(controlOps[1] == OBC::COMM::CSP::DownlinkControlV3Op::BEGIN);
    assert(controlOps[2] == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL);
    assert(controlOps[3] == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT);
    assert(dataFrames.size() == 2U);
    assert(dataFrames[0].streamId == dataFrames[1].streamId);
    assert(dataFrames[0].frameIndex == 0U);
    assert(dataFrames[1].frameIndex == 1U);
    assert(dataFrames[0].byteCount == OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES);
    assert(dataFrames[1].byteCount == 17U);

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(stats.txChunks == 2U);
    assert(stats.txBytes == payload.size());
    assert(stats.txAcceptedBytes == payload.size());
    assert(stats.txAckedBytes == payload.size());
    assert(stats.txInFlightFrames == 0U);
}

void testCommCspNode5ExactMaxV3PayloadFitsSingleDataFrame() {
    FakeCspRuntime runtime;
    std::vector<OBC::COMM::CSP::DownlinkControlV3Op> controlOps;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    const std::string payload(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES, 'm');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t targetNode, std::uint8_t targetPort, const std::string& raw) {
        assert(targetNode == OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID);
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_DATA_V3));
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&controlOps, &payload](std::uint16_t targetNode,
                                std::uint8_t targetPort,
                                const void* requestData,
                                std::size_t requestSize,
                                void* replyData,
                                std::size_t replyCapacity,
                                std::size_t& replySize,
                                std::uint32_t) {
            assert(targetNode == OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID);
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            assert(requestSize == sizeof(OBC::COMM::CSP::DownlinkControlV3Request));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::DownlinkControlV3Reply));

            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            controlOps.push_back(op);

            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            reply.streamId = request->streamId;
            reply.totalFrames = request->totalFrames;
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL ||
                op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
            }
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);

    assert(controlOps.size() == 4U);
    assert(dataFrames.size() == 1U);
    assert(dataFrames[0].frameCount == 1U);
    assert(dataFrames[0].frameIndex == 0U);
    assert(dataFrames[0].byteCount == OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES);

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(stats.txChunks == 1U);
    assert(stats.txBytes == payload.size());
    assert(stats.txAcceptedBytes == payload.size());
}

void testCommCspNode5V3MaxDataBytesEnvOverrideShrinksFrameSize() {
    ScopedEnvVar overrideBytes("COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES", "2031");

    FakeCspRuntime runtime;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    const std::string payload(2032U, 'o');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t, std::uint8_t targetPort, const std::string& raw) {
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_DATA_V3));
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [](std::uint16_t,
           std::uint8_t targetPort,
           const void* requestData,
           std::size_t,
           void* replyData,
           std::size_t,
           std::size_t& replySize,
           std::uint32_t) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            reply.streamId = request->streamId;
            reply.totalFrames = request->totalFrames;
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL ||
                op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);

    assert(dataFrames.size() == 2U);
    assert(dataFrames[0].byteCount == 2031U);
    assert(dataFrames[1].byteCount == 1U);
}

void testCommCspNode5SocketCanCapsV3WindowAtThreeFrames() {
    ScopedEnvVar transport("CSP_TRANSPORT", "socketcan");

    FakeCspRuntime runtime;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    std::size_t ackPollCount = 0U;
    std::size_t sendCountAtFirstAck = 0U;
    const std::string payload((OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES * 4U) + 11U, 's');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t, std::uint8_t targetPort, const std::string& raw) {
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_DATA_V3));
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&ackPollCount, &sendCountAtFirstAck, &dataFrames, &payload](std::uint16_t,
                                                                      std::uint8_t targetPort,
                                                                      const void* requestData,
                                                                      std::size_t,
                                                                      void* replyData,
                                                                      std::size_t,
                                                                      std::size_t& replySize,
                                                                      std::uint32_t) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            reply.streamId = request->streamId;
            reply.totalFrames = request->totalFrames;
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL) {
                ackPollCount += 1U;
                if (ackPollCount == 1U) {
                    sendCountAtFirstAck = dataFrames.size();
                    reply.contiguousFrames = 3U;
                    reply.contiguousBytes =
                        static_cast<std::uint32_t>(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES * 3U);
                    reply.windowCredit = 3U;
                } else {
                    reply.contiguousFrames = request->totalFrames;
                    reply.contiguousBytes = request->totalBytes;
                    reply.windowCredit = 0U;
                }
            } else if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
                reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);

    assert(ackPollCount == 2U);
    assert(sendCountAtFirstAck == 3U);
    assert(dataFrames.size() == 5U);
    assert(dataFrames[0].frameIndex == 0U);
    assert(dataFrames[1].frameIndex == 1U);
    assert(dataFrames[2].frameIndex == 2U);
    assert(dataFrames[3].frameIndex == 3U);
    assert(dataFrames[4].frameIndex == 4U);
    assert(dataFrames[0].byteCount == OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES);
}

void testCommCspNode5WindowOverrideForcesSingleInFlightFrame() {
    ScopedEnvVar overrideWindow("COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE", "1");

    FakeCspRuntime runtime;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    std::size_t ackPollCount = 0U;
    std::size_t sendCountAtFirstAck = 0U;
    const std::string payload((OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES * 2U) + 7U, 'w');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t, std::uint8_t targetPort, const std::string& raw) {
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_DATA_V3));
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&ackPollCount, &sendCountAtFirstAck, &dataFrames, &payload](std::uint16_t,
                                                                     std::uint8_t targetPort,
                                                                     const void* requestData,
                                                                     std::size_t,
                                                                     void* replyData,
                                                                     std::size_t,
                                                                     std::size_t& replySize,
                                                                     std::uint32_t) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            reply.streamId = request->streamId;
            reply.totalFrames = request->totalFrames;
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL) {
                ackPollCount += 1U;
                if (ackPollCount == 1U) {
                    sendCountAtFirstAck = dataFrames.size();
                    reply.contiguousFrames = 1U;
                    reply.contiguousBytes = static_cast<std::uint32_t>(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES);
                    reply.windowCredit = 1U;
                } else if (ackPollCount == 2U) {
                    reply.contiguousFrames = 2U;
                    reply.contiguousBytes = static_cast<std::uint32_t>(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES * 2U);
                    reply.windowCredit = 1U;
                } else {
                    reply.contiguousFrames = request->totalFrames;
                    reply.contiguousBytes = request->totalBytes;
                    reply.windowCredit = 0U;
                }
            } else if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
                reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);

    assert(ackPollCount == 3U);
    assert(sendCountAtFirstAck == 1U);
    assert(dataFrames.size() == 3U);
    assert(dataFrames[0].frameIndex == 0U);
    assert(dataFrames[1].frameIndex == 1U);
    assert(dataFrames[2].frameIndex == 2U);
}

void testCommCspNode5ProbeFailureFallsBackToV1() {
    FakeCspRuntime runtime;
    bool sawV1Send = false;
    bool sawRawData = false;

    runtime.sendRawHandler = [&sawRawData](std::uint16_t, std::uint8_t, const std::string&) {
        sawRawData = true;
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&sawV1Send](std::uint16_t,
                     std::uint8_t targetPort,
                     const void* requestData,
                     std::size_t requestSize,
                     void* replyData,
                     std::size_t,
                     std::size_t& replySize,
                     std::uint32_t) {
            if (targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3)) {
                return OBC::CSP::RuntimeStatus::TIMEOUT;
            }

            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkWriteRequest*>(requestData);
            assert(requestSize == sizeof(OBC::COMM::CSP::DownlinkWriteRequest));
            sawV1Send = true;
            OBC::COMM::CSP::ChunkReply reply =
                OBC::COMM::CSP::makeChunkReply(OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE,
                                               request->header.seq,
                                               OBC::COMM::CSP::ResultCode::OK);
            reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    const std::string payload("fallback-v1");
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);
    assert(sawV1Send);
    assert(!sawRawData);
}

void testCommCspNode5SendRetriesTransientProbeBeforeFallingBack() {
    ScopedEnvVar sendProbeWait("COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_WAIT_MS", "3000");
    ScopedEnvVar sendProbeRetrySleep("COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_RETRY_SLEEP_MS", "0");

    FakeCspRuntime runtime;
    std::vector<OBC::COMM::CSP::DownlinkControlV3Op> controlOps;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    std::size_t statusProbeCount = 0U;
    bool sawV1Send = false;
    const std::string payload(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES + 3U, 'p');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t, std::uint8_t targetPort, const std::string& raw) {
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_DATA_V3));
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&statusProbeCount, &controlOps, &sawV1Send, &payload](std::uint16_t,
                                                               std::uint8_t targetPort,
                                                               const void* requestData,
                                                               std::size_t requestSize,
                                                               void* replyData,
                                                               std::size_t replyCapacity,
                                                               std::size_t& replySize,
                                                               std::uint32_t) {
            if (targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3)) {
                assert(requestSize == sizeof(OBC::COMM::CSP::DownlinkControlV3Request));
                assert(replyCapacity >= sizeof(OBC::COMM::CSP::DownlinkControlV3Reply));
                const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
                const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
                if (op == OBC::COMM::CSP::DownlinkControlV3Op::STATUS) {
                    statusProbeCount += 1U;
                    if (statusProbeCount <= 2U) {
                        return OBC::CSP::RuntimeStatus::TIMEOUT;
                    }
                }

                controlOps.push_back(op);
                OBC::COMM::CSP::DownlinkControlV3Reply reply =
                    makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
                reply.streamId = request->streamId;
                reply.totalFrames = request->totalFrames;
                if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL ||
                    op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                    reply.contiguousFrames = request->totalFrames;
                    reply.contiguousBytes = request->totalBytes;
                }
                if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                    reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
                }
                std::memcpy(replyData, &reply, sizeof(reply));
                replySize = sizeof(reply);
                return OBC::CSP::RuntimeStatus::OK;
            }

            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE));
            sawV1Send = true;
            OBC::COMM::CSP::ChunkReply reply =
                OBC::COMM::CSP::makeChunkReply(OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE, 1U, OBC::COMM::CSP::ResultCode::OK);
            reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);
    assert(statusProbeCount == 3U);
    assert(!sawV1Send);
    assert(controlOps.size() == 4U);
    assert(controlOps[0] == OBC::COMM::CSP::DownlinkControlV3Op::STATUS);
    assert(controlOps[1] == OBC::COMM::CSP::DownlinkControlV3Op::BEGIN);
    assert(controlOps[2] == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL);
    assert(controlOps[3] == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT);
    assert(dataFrames.size() == 2U);
}

void testCommCspNode5TransientProbeTimeoutDoesNotPermanentlyDisableV3() {
    FakeCspRuntime runtime;
    std::vector<OBC::COMM::CSP::DownlinkControlV3Op> controlOps;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    std::size_t statusProbeCount = 0U;
    bool sawV1Send = false;
    const std::string payload(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES + 9U, 't');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t, std::uint8_t targetPort, const std::string& raw) {
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_DATA_V3));
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&statusProbeCount, &controlOps, &sawV1Send, &payload](std::uint16_t,
                                                               std::uint8_t targetPort,
                                                               const void* requestData,
                                                               std::size_t requestSize,
                                                               void* replyData,
                                                               std::size_t replyCapacity,
                                                               std::size_t& replySize,
                                                               std::uint32_t) {
            if (targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::LINK_STATUS)) {
                assert(requestSize == sizeof(OBC::COMM::CSP::LinkStatusRequest));
                assert(replyCapacity >= sizeof(OBC::COMM::CSP::LinkStatusReply));
                const auto* request = static_cast<const OBC::COMM::CSP::LinkStatusRequest*>(requestData);
                OBC::COMM::CSP::LinkStatusReply reply =
                    OBC::COMM::CSP::makeLinkStatusReply(request->header.seq, OBC::COMM::CSP::ResultCode::OK);
                reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
                std::memcpy(replyData, &reply, sizeof(reply));
                replySize = sizeof(reply);
                return OBC::CSP::RuntimeStatus::OK;
            }

            if (targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3)) {
                assert(requestSize == sizeof(OBC::COMM::CSP::DownlinkControlV3Request));
                assert(replyCapacity >= sizeof(OBC::COMM::CSP::DownlinkControlV3Reply));
                const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
                const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
                if (op == OBC::COMM::CSP::DownlinkControlV3Op::STATUS) {
                    statusProbeCount += 1U;
                    if (statusProbeCount <= 20U) {
                        return OBC::CSP::RuntimeStatus::TIMEOUT;
                    }
                }

                controlOps.push_back(op);
                OBC::COMM::CSP::DownlinkControlV3Reply reply =
                    makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
                reply.streamId = request->streamId;
                reply.totalFrames = request->totalFrames;
                if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL ||
                    op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                    reply.contiguousFrames = request->totalFrames;
                    reply.contiguousBytes = request->totalBytes;
                }
                if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                    reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
                }
                std::memcpy(replyData, &reply, sizeof(reply));
                replySize = sizeof(reply);
                return OBC::CSP::RuntimeStatus::OK;
            }

            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkWriteRequest*>(requestData);
            assert(requestSize == sizeof(OBC::COMM::CSP::DownlinkWriteRequest));
            sawV1Send = true;
            OBC::COMM::CSP::ChunkReply reply =
                OBC::COMM::CSP::makeChunkReply(OBC::COMM::CSP::ServicePort::DOWNLINK_WRITE,
                                               request->header.seq,
                                               OBC::COMM::CSP::ResultCode::OK);
            reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.start());
    for (std::size_t i = 0U; i < 19U; ++i) {
        assert(backend.observeHealth());
    }

    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);
    assert(!sawV1Send);
    assert(statusProbeCount == 21U);
    assert(controlOps.size() == 4U);
    assert(controlOps[0] == OBC::COMM::CSP::DownlinkControlV3Op::STATUS);
    assert(controlOps[1] == OBC::COMM::CSP::DownlinkControlV3Op::BEGIN);
    assert(controlOps[2] == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL);
    assert(controlOps[3] == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT);
    assert(dataFrames.size() == 2U);
}

void testCommCspNode5ResendsOnlyTailAfterNoProgress() {
    FakeCspRuntime runtime;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    std::size_t ackPollCount = 0U;
    const std::string payload(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES + 20U, 'r');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t, std::uint8_t targetPort, const std::string& raw) {
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_DATA_V3));
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&ackPollCount, &payload](std::uint16_t,
                                  std::uint8_t targetPort,
                                  const void* requestData,
                                  std::size_t,
                                  void* replyData,
                                  std::size_t,
                                  std::size_t& replySize,
                                  std::uint32_t) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            reply.streamId = request->streamId;
            reply.totalFrames = request->totalFrames;
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL) {
                ackPollCount += 1U;
                if (ackPollCount == 1U) {
                    reply.contiguousFrames = 1U;
                    reply.contiguousBytes = static_cast<std::uint32_t>(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES);
                } else if (ackPollCount == 2U) {
                    reply.header.result = static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::NO_PROGRESS);
                    reply.contiguousFrames = 1U;
                    reply.contiguousBytes = static_cast<std::uint32_t>(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES);
                } else {
                    reply.contiguousFrames = 2U;
                    reply.contiguousBytes = static_cast<std::uint32_t>(payload.size());
                    reply.windowCredit = 0U;
                }
            } else if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
                reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);

    assert(dataFrames.size() == 3U);
    assert(dataFrames[0].frameIndex == 0U);
    assert(dataFrames[1].frameIndex == 1U);
    assert(dataFrames[2].frameIndex == 1U);
    assert(dataFrames[1].byteCount == dataFrames[2].byteCount);

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(stats.txChunks == 3U);
    assert(stats.txBytes == payload.size() + 20U);
    assert(stats.txResentFrames == 1U);
    assert(stats.txAckedBytes == payload.size());
}

void testCommCspNode5RetriesCommitWithoutResendingData() {
    FakeCspRuntime runtime;
    std::size_t commitCount = 0U;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    const std::string payload(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES + 5U, 'c');

    runtime.sendRawHandler = [&dataFrames](std::uint16_t, std::uint8_t, const std::string& raw) {
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&commitCount, &payload](std::uint16_t,
                                 std::uint8_t targetPort,
                                 const void* requestData,
                                 std::size_t,
                                 void* replyData,
                                 std::size_t,
                                 std::size_t& replySize,
                                 std::uint32_t) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            reply.streamId = request->streamId;
            reply.totalFrames = request->totalFrames;
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL) {
                reply.contiguousFrames = 2U;
                reply.contiguousBytes = static_cast<std::uint32_t>(payload.size());
                reply.windowCredit = 0U;
            } else if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                commitCount += 1U;
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
                if (commitCount == 1U) {
                    reply.header.result = static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::NO_CREDIT);
                    reply.drainFreeFrames = 0U;
                } else {
                    reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
                }
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);
    assert(dataFrames.size() == 2U);
    assert(commitCount == 2U);
}

void testCommCspNode5RetriesBeginWithSameStreamAfterLostReply() {
    FakeCspRuntime runtime;
    std::vector<std::uint16_t> beginStreamIds;
    std::vector<OBC::COMM::CSP::DownlinkDataV3Frame> dataFrames;
    const std::string payload("begin-retry");

    runtime.sendRawHandler = [&dataFrames](std::uint16_t, std::uint8_t, const std::string& raw) {
        dataFrames.push_back(decodeDownlinkV3Frame(raw));
        return OBC::CSP::RuntimeStatus::OK;
    };
    runtime.requestReplyHandler =
        [&beginStreamIds, &payload](std::uint16_t,
                                    std::uint8_t targetPort,
                                    const void* requestData,
                                    std::size_t,
                                    void* replyData,
                                    std::size_t,
                                    std::size_t& replySize,
                                    std::uint32_t) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::BEGIN) {
                beginStreamIds.push_back(request->streamId);
                if (beginStreamIds.size() == 1U) {
                    return OBC::CSP::RuntimeStatus::TIMEOUT;
                }
            }
            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            reply.streamId = request->streamId;
            reply.totalFrames = request->totalFrames;
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL ||
                op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.contiguousFrames = request->totalFrames;
                reply.contiguousBytes = request->totalBytes;
            }
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::COMMIT) {
                reply.acceptedBytes = static_cast<std::uint32_t>(payload.size());
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);
    assert(beginStreamIds.size() == 2U);
    assert(beginStreamIds[0] == beginStreamIds[1]);
    assert(dataFrames.size() == 1U);
}

void testCommCspNode5DoesNotRetryBeginExecutionError() {
    FakeCspRuntime runtime;
    std::size_t beginCount = 0U;
    runtime.requestReplyHandler = [&beginCount](std::uint16_t,
                                                std::uint8_t targetPort,
                                                const void* requestData,
                                                std::size_t,
                                                void* replyData,
                                                std::size_t,
                                                std::size_t& replySize,
                                                std::uint32_t) {
        assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
        const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
        const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
        if (op == OBC::COMM::CSP::DownlinkControlV3Op::STATUS) {
            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        }
        assert(op == OBC::COMM::CSP::DownlinkControlV3Op::BEGIN);
        beginCount += 1U;
        return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    const std::string payload("begin-error");
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::RETRY);
    assert(beginCount == 1U);
}

void testCommCspNode5AbortsV3StreamAfterMidStreamFailure() {
    FakeCspRuntime runtime;
    std::size_t sendRawCount = 0U;
    std::size_t abortCount = 0U;
    std::uint16_t observedStreamId = 0U;
    const std::string payload(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES + 12U, 'a');

    runtime.sendRawHandler = [&sendRawCount](std::uint16_t, std::uint8_t, const std::string&) {
        sendRawCount += 1U;
        return sendRawCount == 1U ? OBC::CSP::RuntimeStatus::OK : OBC::CSP::RuntimeStatus::TIMEOUT;
    };
    runtime.requestReplyHandler =
        [&abortCount, &observedStreamId](std::uint16_t,
                                         std::uint8_t targetPort,
                                         const void* requestData,
                                         std::size_t,
                                         void* replyData,
                                         std::size_t,
                                         std::size_t& replySize,
                                         std::uint32_t) {
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3));
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkControlV3Request*>(requestData);
            const auto op = static_cast<OBC::COMM::CSP::DownlinkControlV3Op>(request->op);
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::BEGIN) {
                observedStreamId = request->streamId;
            }
            OBC::COMM::CSP::DownlinkControlV3Reply reply =
                makeDownlinkV3Reply(request->header.seq, op, OBC::COMM::CSP::ResultCode::OK);
            if (op == OBC::COMM::CSP::DownlinkControlV3Op::ABORT) {
                assert(request->streamId == observedStreamId);
                abortCount += 1U;
            }
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID, runtime);
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::RETRY);
    assert(abortCount == 1U);
}

void testCommCspV2OnlyPolicyDoesNotProbeOrSendV3() {
    FakeCspRuntime runtime;
    std::size_t v2StatusCount = 0U;
    std::size_t v2StageCount = 0U;
    bool sawV3 = false;

    runtime.requestReplyHandler =
        [&v2StatusCount, &v2StageCount, &sawV3](std::uint16_t,
                                                std::uint8_t targetPort,
                                                const void* requestData,
                                                std::size_t,
                                                void* replyData,
                                                std::size_t,
                                                std::size_t& replySize,
                                                std::uint32_t) {
            if (targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::LINK_STATUS)) {
                const auto* request = static_cast<const OBC::COMM::CSP::LinkStatusRequest*>(requestData);
                OBC::COMM::CSP::LinkStatusReply reply =
                    OBC::COMM::CSP::makeLinkStatusReply(request->header.seq, OBC::COMM::CSP::ResultCode::OK);
                reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
                std::memcpy(replyData, &reply, sizeof(reply));
                replySize = sizeof(reply);
                return OBC::CSP::RuntimeStatus::OK;
            }
            if (targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_CONTROL_V3)) {
                sawV3 = true;
                return OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
            }
            if (targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_STATUS_V2)) {
                v2StatusCount += 1U;
                const auto* request = static_cast<const OBC::COMM::CSP::DownlinkStatusV2Request*>(requestData);
                OBC::COMM::CSP::DownlinkStatusV2Reply reply =
                    OBC::COMM::CSP::makeDownlinkStatusV2Reply(request->header.seq, OBC::COMM::CSP::ResultCode::OK);
                reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
                std::memcpy(replyData, &reply, sizeof(reply));
                replySize = sizeof(reply);
                return OBC::CSP::RuntimeStatus::OK;
            }
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::DOWNLINK_STAGE_V2));
            v2StageCount += 1U;
            const auto* request = static_cast<const OBC::COMM::CSP::DownlinkStageV2Request*>(requestData);
            OBC::COMM::CSP::DownlinkStageV2Reply reply =
                OBC::COMM::CSP::makeDownlinkStageV2Reply(request->header.seq, OBC::COMM::CSP::ResultCode::OK);
            reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
            reply.drainFreeSlots = OBC::COMM::CSP::DOWNLINK_STAGE_V2_DRAIN_SLOTS;
            std::memcpy(replyData, &reply, sizeof(reply));
            replySize = sizeof(reply);
            return OBC::CSP::RuntimeStatus::OK;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_SBAND_COMM_NODE_ID,
                                                runtime,
                                                OBC::COMM::GroundLinkHealthSemantics::DISABLED,
                                                OBC::COMM::CommCspDownlinkPolicy::V2_ONLY);
    const std::string payload(OBC::COMM::CSP::DOWNLINK_STAGE_V2_MAX_DATA_BYTES + 1U, 'v');
    assert(backend.start());
    assert(backend.send(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()) ==
           OBC::COMM::GroundLinkSendStatus::OK);
    assert(!sawV3);
    assert(v2StatusCount == 3U);
    assert(v2StageCount == 2U);
}

void testCommNodeDownlinkV2StateHappyPathStagesCommitsAndFlushes() {
    OBC::COMM::CommNodeDownlinkV2State state;
    const std::string firstPayload("HELLO");
    const std::string lastPayload("WORLD!");

    const auto firstReply = state.handleStage(makeDownlinkStageRequest(7U, 1U, firstPayload, true, false),
                                              OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(firstReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(firstReply.stagedSlotsUsed == 1U);
    assert(firstReply.drainFreeSlots == OBC::COMM::CSP::DOWNLINK_STAGE_V2_DRAIN_SLOTS);

    const auto lastReply = state.handleStage(makeDownlinkStageRequest(7U, 2U, lastPayload, false, true),
                                             OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(lastReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(lastReply.stagedSlotsUsed == 0U);
    assert(lastReply.drainFreeSlots == OBC::COMM::CSP::DOWNLINK_STAGE_V2_DRAIN_SLOTS - 2U);

    auto status = state.handleStatus(OBC::COMM::CSP::makeDownlinkStatusV2Request(11U),
                                     OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.drainQueuedSlots == 2U);
    assert(status.stagingActive == 0U);
    assert(status.acceptedBytes == firstPayload.size() + lastPayload.size());
    assert(status.flushedBytes == 0U);

    OBC::COMM::CommNodeDownlinkV2State::DrainChunk chunk = {};
    assert(state.peekDrainChunk(chunk));
    assert(chunk.streamId == 7U);
    assert(chunk.acceptedSeq == 1U);
    assert(state.confirmDrainChunkFlushed(chunk));
    assert(state.peekDrainChunk(chunk));
    assert(chunk.acceptedSeq == 2U);
    assert(state.confirmDrainChunkFlushed(chunk));

    status = state.handleStatus(OBC::COMM::CSP::makeDownlinkStatusV2Request(12U),
                                OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.drainQueuedSlots == 0U);
    assert(status.flushedBytes == firstPayload.size() + lastPayload.size());
}

void testCommNodeDownlinkV2StateDuplicateChunkReturnsCachedReply() {
    OBC::COMM::CommNodeDownlinkV2State state;
    const auto request = makeDownlinkStageRequest(9U, 1U, "ABC", true, false);
    const auto firstReply = state.handleStage(request, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    const auto duplicateReply = state.handleStage(request, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(firstReply.header.result == duplicateReply.header.result);
    assert(firstReply.stagedSlotsUsed == duplicateReply.stagedSlotsUsed);

    const auto finalReply = state.handleStage(makeDownlinkStageRequest(9U, 2U, "DEF", false, true),
                                              OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(finalReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));

    const auto status = state.handleStatus(OBC::COMM::CSP::makeDownlinkStatusV2Request(13U),
                                           OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.drainQueuedSlots == 2U);
    assert(status.acceptedBytes == 6U);
}

void testCommNodeDownlinkV2StateFreshStreamDoesNotReplayCommittedReply() {
    OBC::COMM::CommNodeDownlinkV2State state;
    bool acceptedNewChunk = false;
    const auto oldRequest = makeDownlinkStageRequest(1U, 1U, "OLD", true, true);
    const auto oldReply =
        state.handleStage(oldRequest, OBC::COMM::CSP::FLAG_LINK_CONNECTED, &acceptedNewChunk);
    assert(oldReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(acceptedNewChunk);

    acceptedNewChunk = true;
    const auto duplicateReply =
        state.handleStage(oldRequest, OBC::COMM::CSP::FLAG_LINK_CONNECTED, &acceptedNewChunk);
    assert(duplicateReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(!acceptedNewChunk);

    const auto freshRequest = makeDownlinkStageRequest(1U, 1U, "NEW", true, true);
    const auto freshReply =
        state.handleStage(freshRequest, OBC::COMM::CSP::FLAG_LINK_CONNECTED, &acceptedNewChunk);
    assert(freshReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(acceptedNewChunk);

    auto status = state.handleStatus(OBC::COMM::CSP::makeDownlinkStatusV2Request(14U),
                                     OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.acceptedBytes == 6U);
    assert(status.drainQueuedSlots == 2U);

    OBC::COMM::CommNodeDownlinkV2State::DrainChunk chunk = {};
    assert(state.peekDrainChunk(chunk));
    assert(state.confirmDrainChunkFlushed(chunk));
    assert(state.peekDrainChunk(chunk));
    assert(state.confirmDrainChunkFlushed(chunk));

    acceptedNewChunk = false;
    const auto repeatedAfterFlush =
        state.handleStage(freshRequest, OBC::COMM::CSP::FLAG_LINK_CONNECTED, &acceptedNewChunk);
    assert(repeatedAfterFlush.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(acceptedNewChunk);
    status = state.handleStatus(OBC::COMM::CSP::makeDownlinkStatusV2Request(15U),
                                OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.acceptedBytes == 9U);
}

void testCommNodeDownlinkV2StateRejectsInvalidRequestWithoutMutation() {
    OBC::COMM::CommNodeDownlinkV2State state;
    const auto firstReply = state.handleStage(makeDownlinkStageRequest(11U, 1U, "AAA", true, false),
                                              OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(firstReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));

    const auto invalidReply = state.handleStage(makeDownlinkStageRequest(12U, 2U, "BBB", false, true),
                                                OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(invalidReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::INVALID_REQUEST));

    const auto status = state.handleStatus(OBC::COMM::CSP::makeDownlinkStatusV2Request(14U),
                                           OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.stagingActive == 1U);
    assert(status.stagingStreamId == 11U);
    assert(status.drainQueuedSlots == 0U);
    assert(status.acceptedBytes == 0U);
}

void testCommNodeDownlinkV2StateDisconnectPurgesCommittedAndStagingBytes() {
    OBC::COMM::CommNodeDownlinkV2State state;
    static_cast<void>(state.handleStage(makeDownlinkStageRequest(21U, 1U, "HELLO", true, false),
                                        OBC::COMM::CSP::FLAG_LINK_CONNECTED));
    static_cast<void>(state.handleStage(makeDownlinkStageRequest(21U, 2U, "WORLD", false, true),
                                        OBC::COMM::CSP::FLAG_LINK_CONNECTED));
    static_cast<void>(state.handleStage(makeDownlinkStageRequest(22U, 1U, "PEND", true, false),
                                        OBC::COMM::CSP::FLAG_LINK_CONNECTED));

    state.handleDrainWriteFailure(0U);
    const auto status = state.handleStatus(OBC::COMM::CSP::makeDownlinkStatusV2Request(15U), 0U);
    assert(status.drainQueuedSlots == 0U);
    assert(status.stagingActive == 0U);
    assert(status.acceptedBytes == 10U);
    assert(status.droppedCommittedBytes == 10U);
}

void testCommNodeDownlinkV3StateHappyPathCommitsAndFlushes() {
    OBC::COMM::CommNodeDownlinkV3State state;
    const std::string firstPayload(OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES, 'h');
    const std::string secondPayload("TAIL-V3");

    auto begin = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::BEGIN, 1U, 41U);
    begin.totalFrames = 2U;
    begin.totalBytes = static_cast<std::uint32_t>(firstPayload.size() + secondPayload.size());
    const auto beginReply = state.handleControl(begin, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(beginReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(beginReply.stagingActive == 1U);

    auto firstFrame = OBC::COMM::CSP::makeDownlinkDataV3Frame(41U);
    firstFrame.frameIndex = 0U;
    firstFrame.frameCount = 2U;
    firstFrame.byteOffset = 0U;
    firstFrame.byteCount = static_cast<std::uint16_t>(firstPayload.size());
    std::memcpy(firstFrame.data, firstPayload.data(), firstPayload.size());
    assert(state.handleData(firstFrame));

    auto secondFrame = OBC::COMM::CSP::makeDownlinkDataV3Frame(41U);
    secondFrame.frameIndex = 1U;
    secondFrame.frameCount = 2U;
    secondFrame.byteOffset = static_cast<std::uint32_t>(firstPayload.size());
    secondFrame.byteCount = static_cast<std::uint16_t>(secondPayload.size());
    std::memcpy(secondFrame.data, secondPayload.data(), secondPayload.size());
    assert(state.handleData(secondFrame));

    auto ack = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::ACK_POLL, 2U, 41U);
    ack.totalFrames = 2U;
    ack.totalBytes = begin.totalBytes;
    const auto ackReply = state.handleControl(ack, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(ackReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(ackReply.contiguousFrames == 2U);
    assert(ackReply.contiguousBytes == begin.totalBytes);

    auto commit = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::COMMIT, 3U, 41U);
    commit.totalFrames = 2U;
    commit.totalBytes = begin.totalBytes;
    const auto commitReply = state.handleControl(commit, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(commitReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(commitReply.acceptedBytes == begin.totalBytes);
    assert(commitReply.drainQueuedFrames == 2U);

    OBC::COMM::CommNodeDownlinkV3State::DrainFrame frame = {};
    assert(state.peekDrainFrame(frame));
    assert(frame.frameIndex == 0U);
    assert(state.confirmDrainFrameFlushed(frame));
    assert(state.peekDrainFrame(frame));
    assert(frame.frameIndex == 1U);
    assert(state.confirmDrainFrameFlushed(frame));

    const auto status = state.handleControl(
        OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::STATUS, 4U, 0U),
        OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.stagingActive == 0U);
    assert(status.acceptedBytes == begin.totalBytes);
    assert(status.flushedBytes == begin.totalBytes);
}

void testCommNodeDownlinkV3StateAcceptsVariableFrameSizesBelowMax() {
    OBC::COMM::CommNodeDownlinkV3State state;
    constexpr std::uint16_t kFrameBytes = OBC::COMM::CSP::DOWNLINK_V3_MAX_DATA_BYTES;
    constexpr std::uint32_t kTotalBytes = 4096U;
    constexpr std::uint16_t kTotalFrames = static_cast<std::uint16_t>((kTotalBytes + kFrameBytes - 1U) / kFrameBytes);

    auto begin = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::BEGIN, 1U, 77U);
    begin.totalFrames = kTotalFrames;
    begin.totalBytes = kTotalBytes;
    const auto beginReply = state.handleControl(begin, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(beginReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));

    std::uint32_t offset = 0U;
    for (std::uint16_t frameIndex = 0U; frameIndex < kTotalFrames; ++frameIndex) {
        auto frame = OBC::COMM::CSP::makeDownlinkDataV3Frame(77U);
        frame.frameIndex = frameIndex;
        frame.frameCount = kTotalFrames;
        frame.byteOffset = offset;
        frame.byteCount = static_cast<std::uint16_t>(std::min<std::uint32_t>(kFrameBytes, kTotalBytes - offset));
        std::memset(frame.data, 'z', frame.byteCount);
        assert(state.handleData(frame));
        offset += frame.byteCount;
    }
    assert(offset == kTotalBytes);

    auto commit = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::COMMIT, 2U, 77U);
    commit.totalFrames = kTotalFrames;
    commit.totalBytes = kTotalBytes;
    const auto commitReply = state.handleControl(commit, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(commitReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(commitReply.contiguousFrames == kTotalFrames);
    assert(commitReply.contiguousBytes == kTotalBytes);
    assert(commitReply.acceptedBytes == kTotalBytes);
    assert(commitReply.drainQueuedFrames == kTotalFrames);
}

void testCommNodeDownlinkV3StateDuplicateBeginCommitAndDataAreIdempotent() {
    OBC::COMM::CommNodeDownlinkV3State state;
    auto begin = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::BEGIN, 1U, 9U);
    begin.totalFrames = 1U;
    begin.totalBytes = 3U;
    const auto firstBeginReply = state.handleControl(begin, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    const auto duplicateBeginReply = state.handleControl(begin, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(firstBeginReply.header.result == duplicateBeginReply.header.result);

    auto frame = OBC::COMM::CSP::makeDownlinkDataV3Frame(9U);
    frame.frameIndex = 0U;
    frame.frameCount = 1U;
    frame.byteOffset = 0U;
    frame.byteCount = 3U;
    std::memcpy(frame.data, "XYZ", 3U);
    assert(state.handleData(frame));
    assert(!state.handleData(frame));

    auto commit = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::COMMIT, 2U, 9U);
    commit.totalFrames = 1U;
    commit.totalBytes = 3U;
    const auto firstCommitReply = state.handleControl(commit, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    const auto duplicateCommitReply = state.handleControl(commit, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(firstCommitReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(duplicateCommitReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));

    auto freshBegin = begin;
    freshBegin.header.seq = 3U;
    const auto freshBeginReply = state.handleControl(freshBegin, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(freshBeginReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::OK));
    assert(freshBeginReply.stagingActive == 1U);
    assert(freshBeginReply.stagingStreamId == 9U);

    const auto status = state.handleControl(
        OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::STATUS, 4U, 0U),
        OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.drainQueuedFrames == 1U);
    assert(status.acceptedBytes == 3U);
    assert(status.stagingActive == 1U);
}

void testCommNodeDownlinkV3StateRejectsWrongStreamAndTimesOutCleanly() {
    OBC::COMM::CommNodeDownlinkV3State state;
    auto begin = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::BEGIN, 1U, 17U);
    begin.totalFrames = 2U;
    begin.totalBytes = 1004U;
    static_cast<void>(state.handleControl(begin, OBC::COMM::CSP::FLAG_LINK_CONNECTED));

    auto firstFrame = OBC::COMM::CSP::makeDownlinkDataV3Frame(17U);
    firstFrame.frameIndex = 0U;
    firstFrame.frameCount = 2U;
    firstFrame.byteOffset = 0U;
    firstFrame.byteCount = 1000U;
    std::memset(firstFrame.data, 'q', firstFrame.byteCount);
    assert(state.handleData(firstFrame));

    auto invalidCommit = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::COMMIT, 2U, 18U);
    invalidCommit.totalFrames = 2U;
    invalidCommit.totalBytes = 1004U;
    const auto invalidReply = state.handleControl(invalidCommit, OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(invalidReply.header.result == static_cast<std::uint8_t>(OBC::COMM::CSP::ResultCode::INVALID_REQUEST));

    state.reapTimedOut(std::chrono::steady_clock::now() + std::chrono::milliseconds(2500));
    const auto status = state.handleControl(
        OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::STATUS, 3U, 0U),
        OBC::COMM::CSP::FLAG_LINK_CONNECTED);
    assert(status.stagingActive == 0U);
    assert(status.drainQueuedFrames == 0U);
}

void testCommNodeDownlinkV3StateDrainFailurePurgesCommittedBytes() {
    OBC::COMM::CommNodeDownlinkV3State state;
    auto begin = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::BEGIN, 1U, 23U);
    begin.totalFrames = 1U;
    begin.totalBytes = 4U;
    static_cast<void>(state.handleControl(begin, OBC::COMM::CSP::FLAG_LINK_CONNECTED));

    auto frame = OBC::COMM::CSP::makeDownlinkDataV3Frame(23U);
    frame.frameIndex = 0U;
    frame.frameCount = 1U;
    frame.byteOffset = 0U;
    frame.byteCount = 4U;
    std::memcpy(frame.data, "DATA", 4U);
    assert(state.handleData(frame));

    auto commit = OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::COMMIT, 2U, 23U);
    commit.totalFrames = 1U;
    commit.totalBytes = 4U;
    static_cast<void>(state.handleControl(commit, OBC::COMM::CSP::FLAG_LINK_CONNECTED));

    state.handleDrainWriteFailure(0U);
    const auto status = state.handleControl(
        OBC::COMM::CSP::makeDownlinkControlV3Request(OBC::COMM::CSP::DownlinkControlV3Op::STATUS, 3U, 0U),
        0U);
    assert(status.drainQueuedFrames == 0U);
    assert(status.stagingActive == 0U);
    assert(status.acceptedBytes == 4U);
    assert(status.droppedCommittedBytes == 4U);
}

void testDownlinkDataV3DeserializeAcceptsCanFdPaddedTailFrame() {
    auto frame = OBC::COMM::CSP::makeDownlinkDataV3Frame(31U);
    frame.frameIndex = 2U;
    frame.frameCount = 3U;
    frame.byteOffset = 4064U;
    frame.byteCount = 32U;
    for (std::uint16_t i = 0; i < frame.byteCount; i++) {
        frame.data[i] = static_cast<std::uint8_t>('A' + (i % 26U));
    }

    std::string payload;
    assert(OBC::COMM::CSP::serializeDownlinkDataV3Frame(frame, payload));
    assert(payload.size() == OBC::COMM::CSP::downlinkDataV3SerializedSize(frame.byteCount));

    // CAN FD may pad 52-byte on-wire frames up to a 64-byte DLC. The
    // application-level frame carries its own byteCount, so trailing pad bytes
    // must not make the V3 decode fail.
    const std::size_t paddedSize = 60U;
    assert(paddedSize > payload.size());
    payload.resize(paddedSize, static_cast<char>(0x44));

    OBC::COMM::CSP::DownlinkDataV3Frame decoded = {};
    assert(OBC::COMM::CSP::deserializeDownlinkDataV3Frame(payload.data(), payload.size(), decoded));
    assert(decoded.streamId == frame.streamId);
    assert(decoded.frameIndex == frame.frameIndex);
    assert(decoded.frameCount == frame.frameCount);
    assert(decoded.byteOffset == frame.byteOffset);
    assert(decoded.byteCount == frame.byteCount);
    assert(std::memcmp(decoded.data, frame.data, frame.byteCount) == 0);
}

void testGenericCommNodeTimeoutFallsBackToIdle() {
    FakeCspRuntime runtime;
    std::size_t requestCount = 0U;
    runtime.requestReplyHandler =
        [&requestCount](std::uint16_t targetNode,
                        std::uint8_t targetPort,
                        const void* requestData,
                        std::size_t requestSize,
                        void* replyData,
                        std::size_t replyCapacity,
                        std::size_t& replySize,
                        std::uint32_t timeoutMs) {
            assert(targetNode == OBC::COMM::CSP::DEFAULT_GENERIC_COMM_NODE_ID);
            assert(targetPort == static_cast<std::uint8_t>(OBC::COMM::CSP::ServicePort::UPLINK_POLL));
            assert(requestSize == sizeof(OBC::COMM::CSP::UplinkPollRequest));
            assert(replyCapacity >= sizeof(OBC::COMM::CSP::ChunkReply));
            assert(timeoutMs == 25U);

            if (requestCount == 0U) {
                const auto* request = static_cast<const OBC::COMM::CSP::UplinkPollRequest*>(requestData);
                OBC::COMM::CSP::ChunkReply reply =
                    OBC::COMM::CSP::makeChunkReply(
                        OBC::COMM::CSP::ServicePort::UPLINK_POLL, request->header.seq, OBC::COMM::CSP::ResultCode::NO_CHUNK);
                reply.header.flags = OBC::COMM::CSP::FLAG_LINK_CONNECTED;
                std::memcpy(replyData, &reply, sizeof(reply));
                replySize = sizeof(reply);
                requestCount += 1U;
                return OBC::CSP::RuntimeStatus::OK;
            }

            requestCount += 1U;
            return OBC::CSP::RuntimeStatus::TIMEOUT;
        };

    OBC::COMM::CommCspGroundLinkBackend backend(OBC::COMM::CSP::DEFAULT_GENERIC_COMM_NODE_ID, runtime);
    std::string chunk;
    assert(backend.receive(chunk, 25U) == OBC::COMM::GroundLinkReceiveStatus::IDLE);
    assert(backend.receive(chunk, 25U) == OBC::COMM::GroundLinkReceiveStatus::IDLE);

    const OBC::COMM::GroundLinkStats stats = backend.getStats();
    assert(stats.connected);
    assert(stats.rxErrors == 0U);
    assert(requestCount == 2U);
}

void testGroundGatewayProxyMovesBytesBothDirections() {
    TcpServer server;
    PtyPeer pty;

    OBC::COMM::GroundGatewayConfig config = {};
    config.gdsHost = "127.0.0.1";
    config.gdsPort = server.port();
    config.serialDevice = pty.slavePath();
    config.baudrate = 115200U;

    OBC::COMM::GroundGatewayProxy proxy(config);
    std::thread proxyThread([&proxy]() { proxy.run(); });

    const int clientFd = server.acceptClient();

    const std::string gdsPayload("GDS->COMM");
    assert(writeAll(clientFd, reinterpret_cast<const std::uint8_t*>(gdsPayload.data()), gdsPayload.size()));

    std::string serialPayload;
    assert(readExact(pty.masterFd(), serialPayload, gdsPayload.size()));
    assert(serialPayload == gdsPayload);

    const std::string commPayload("COMM->GDS");
    assert(writeAll(pty.masterFd(), reinterpret_cast<const std::uint8_t*>(commPayload.data()), commPayload.size()));

    std::string observedAtGds;
    assert(readExact(clientFd, observedAtGds, commPayload.size()));
    assert(observedAtGds == commPayload);

    proxy.stop();
    proxyThread.join();
}

void testGroundGatewayProxyMovesBytesOverTcpSouthbound() {
    TcpServer gdsServer;
    TcpServer rfServer;

    OBC::COMM::GroundGatewayConfig config = {};
    config.gdsHost = "127.0.0.1";
    config.gdsPort = gdsServer.port();
    config.southboundMode = OBC::COMM::GroundGatewaySouthboundMode::TCP_CLIENT;
    config.rfTcpHost = "127.0.0.1";
    config.rfTcpPort = rfServer.port();
    config.linkIdentity = "sband";

    OBC::COMM::GroundGatewayProxy proxy(config);
    std::thread proxyThread([&proxy]() { proxy.run(); });

    const int gdsClientFd = gdsServer.acceptClient();
    const int rfClientFd = rfServer.acceptClient();

    const std::string gdsPayload("GDS->SBAND");
    assert(writeAll(gdsClientFd, reinterpret_cast<const std::uint8_t*>(gdsPayload.data()), gdsPayload.size()));

    std::string observedAtRf;
    assert(readExact(rfClientFd, observedAtRf, gdsPayload.size()));
    assert(observedAtRf == gdsPayload);

    const std::string rfPayload("SBAND->GDS");
    assert(writeAll(rfClientFd, reinterpret_cast<const std::uint8_t*>(rfPayload.data()), rfPayload.size()));

    std::string observedAtGds;
    assert(readExact(gdsClientFd, observedAtGds, rfPayload.size()));
    assert(observedAtGds == rfPayload);

    proxy.stop();
    proxyThread.join();
}

void testGroundGatewayProxyCapturesBothDirections() {
    TcpServer gdsServer;
    TcpServer rfServer;

    const std::string base = std::string("/tmp/obc-ground-gateway-capture-") + std::to_string(::getpid());
    const std::string gdsToSouthboundPath = base + "-gds-to-southbound.bin";
    const std::string southboundToGdsPath = base + "-southbound-to-gds.bin";
    (void)::unlink(gdsToSouthboundPath.c_str());
    (void)::unlink(southboundToGdsPath.c_str());

    OBC::COMM::GroundGatewayConfig config = {};
    config.gdsHost = "127.0.0.1";
    config.gdsPort = gdsServer.port();
    config.southboundMode = OBC::COMM::GroundGatewaySouthboundMode::TCP_CLIENT;
    config.rfTcpHost = "127.0.0.1";
    config.rfTcpPort = rfServer.port();
    config.captureGdsToSouthboundPath = gdsToSouthboundPath;
    config.captureSouthboundToGdsPath = southboundToGdsPath;

    OBC::COMM::GroundGatewayProxy proxy(config);
    std::thread proxyThread([&proxy]() { proxy.run(); });

    const int gdsClientFd = gdsServer.acceptClient();
    const int rfClientFd = rfServer.acceptClient();

    const std::string gdsPayload("CAPTURE-GDS->SBAND");
    assert(writeAll(gdsClientFd, reinterpret_cast<const std::uint8_t*>(gdsPayload.data()), gdsPayload.size()));
    std::string observedAtRf;
    assert(readExact(rfClientFd, observedAtRf, gdsPayload.size()));
    assert(observedAtRf == gdsPayload);

    const std::string rfPayload("CAPTURE-SBAND->GDS");
    assert(writeAll(rfClientFd, reinterpret_cast<const std::uint8_t*>(rfPayload.data()), rfPayload.size()));
    std::string observedAtGds;
    assert(readExact(gdsClientFd, observedAtGds, rfPayload.size()));
    assert(observedAtGds == rfPayload);

    proxy.stop();
    proxyThread.join();

    assert(readFile(gdsToSouthboundPath) == gdsPayload);
    assert(readFile(southboundToGdsPath) == rfPayload);
    (void)::unlink(gdsToSouthboundPath.c_str());
    (void)::unlink(southboundToGdsPath.c_str());
}

void testStreamIoTcpWriteAfterPeerCloseReturnsFalse() {
    TcpServer server;
    int clientFd = -1;
    assert(OBC::COMM::openTcpClient("127.0.0.1", server.port(), clientFd));
    static_cast<void>(server.acceptClient());

    server.closeClientWithReset();

    std::uint8_t buffer[1] = {};
    std::size_t bytesRead = 0U;
    static_cast<void>(OBC::COMM::readSome(clientFd, 1000U, buffer, sizeof(buffer), bytesRead));

    const std::string payload("after-close");
    assert(!OBC::COMM::writeAll(clientFd, payload));
    ::close(clientFd);
}

}  // namespace

int main() {
    testCommCspReceiveData();
    testCommCspSendSplitsLargePayload();
    testCommCspStartRefreshesStatus();
    testCommCspIdlePollRefreshesObservationFreshness();
    testActiveCommNodeTimeoutMarksDisconnected();
    testCommCspSendTimeoutRequestsRetryAndClearsConnected();
    testCommCspObservationPublishesConfiguredSemantics();
    testCommCspNode5ProbeSelectsV3AndSendUsesControlPlusRawData();
    testCommCspNode5ExactMaxV3PayloadFitsSingleDataFrame();
    testCommCspNode5V3MaxDataBytesEnvOverrideShrinksFrameSize();
    testCommCspNode5SocketCanCapsV3WindowAtThreeFrames();
    testCommCspNode5WindowOverrideForcesSingleInFlightFrame();
    testCommCspNode5ProbeFailureFallsBackToV1();
    testCommCspNode5SendRetriesTransientProbeBeforeFallingBack();
    testCommCspNode5TransientProbeTimeoutDoesNotPermanentlyDisableV3();
    testCommCspNode5ResendsOnlyTailAfterNoProgress();
    testCommCspNode5RetriesCommitWithoutResendingData();
    testCommCspNode5RetriesBeginWithSameStreamAfterLostReply();
    testCommCspNode5DoesNotRetryBeginExecutionError();
    testCommCspNode5AbortsV3StreamAfterMidStreamFailure();
    testCommCspV2OnlyPolicyDoesNotProbeOrSendV3();
    testCommNodeDownlinkV2StateHappyPathStagesCommitsAndFlushes();
    testCommNodeDownlinkV2StateDuplicateChunkReturnsCachedReply();
    testCommNodeDownlinkV2StateFreshStreamDoesNotReplayCommittedReply();
    testCommNodeDownlinkV2StateRejectsInvalidRequestWithoutMutation();
    testCommNodeDownlinkV2StateDisconnectPurgesCommittedAndStagingBytes();
    testCommNodeDownlinkV3StateHappyPathCommitsAndFlushes();
    testCommNodeDownlinkV3StateAcceptsVariableFrameSizesBelowMax();
    testCommNodeDownlinkV3StateDuplicateBeginCommitAndDataAreIdempotent();
    testCommNodeDownlinkV3StateRejectsWrongStreamAndTimesOutCleanly();
    testCommNodeDownlinkV3StateDrainFailurePurgesCommittedBytes();
    testDownlinkDataV3DeserializeAcceptsCanFdPaddedTailFrame();
    testGenericCommNodeTimeoutFallsBackToIdle();
    testGroundGatewayProxyMovesBytesBothDirections();
    testGroundGatewayProxyMovesBytesOverTcpSouthbound();
    testGroundGatewayProxyCapturesBothDirections();
    testStreamIoTcpWriteAfterPeerCloseReturnsFalse();
    return 0;
}
