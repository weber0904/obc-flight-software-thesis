#ifndef OBC_COMM_GROUNDLINKBACKEND_HPP
#define OBC_COMM_GROUNDLINKBACKEND_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "OBC/Components/GroundLinkDriver/GroundLinkObservationRuntime.hpp"
#include "simulators/comm/CommCspProtocol.hpp"
#include "simulators/csp/CspRuntime.hpp"

namespace OBC {
namespace COMM {

const char* groundLinkBackendModeName(GroundLinkBackendMode mode);

enum class GroundLinkReceiveStatus {
    DATA,
    IDLE,
    DISCONNECTED,
    ERROR,
};

enum class GroundLinkSendStatus {
    OK,
    RETRY,
    ERROR,
};

enum class CommCspDownlinkPolicy {
    AUTO,
    V2_ONLY,
};

struct GroundLinkStats {
    GroundLinkBackendMode mode = GroundLinkBackendMode::DISABLED;
    bool connected = false;
    std::uint32_t txChunks = 0U;
    std::uint32_t rxChunks = 0U;
    std::uint32_t txBytes = 0U;
    std::uint32_t rxBytes = 0U;
    std::uint32_t txErrors = 0U;
    std::uint32_t rxErrors = 0U;
    std::uint32_t txAcceptedBytes = 0U;
    std::uint32_t txFlushedBytes = 0U;
    std::uint32_t txDroppedBytes = 0U;
    std::uint32_t txQueueSlotsUsed = 0U;
    std::uint32_t txAckedBytes = 0U;
    std::uint32_t txResentFrames = 0U;
    std::uint32_t txInFlightFrames = 0U;
};

class IGroundLinkBackend {
  public:
    virtual ~IGroundLinkBackend() = default;

    virtual bool start() = 0;

    virtual void stop() = 0;

    virtual GroundLinkReceiveStatus receive(std::string& outChunk, std::uint32_t timeoutMs) = 0;

    virtual GroundLinkSendStatus send(const std::uint8_t* data, std::size_t size) = 0;

    virtual GroundLinkStats getStats() const = 0;

    virtual GroundLinkObservationState getObservationState() const = 0;

    virtual bool observeHealth() = 0;
};

class DirectTcpGroundLinkBackend final : public IGroundLinkBackend {
  public:
    DirectTcpGroundLinkBackend(std::string host, std::uint16_t port, std::size_t maxChunkBytes = 512U);

    bool start() override;

    void stop() override;

    GroundLinkReceiveStatus receive(std::string& outChunk, std::uint32_t timeoutMs) override;

    GroundLinkSendStatus send(const std::uint8_t* data, std::size_t size) override;

    GroundLinkStats getStats() const override;

    GroundLinkObservationState getObservationState() const override;

    bool observeHealth() override;

  private:
    bool ensureConnectedLocked_();

    void handleDisconnectLocked_();

    std::string m_host;
    std::uint16_t m_port;
    std::vector<std::uint8_t> m_receiveBuffer;
    int m_fd;
    mutable std::mutex m_mutex;
    GroundLinkStats m_stats;
    std::uint32_t m_successfulStatusObservations;
};

class CommCspGroundLinkBackend final : public IGroundLinkBackend {
  public:
    explicit CommCspGroundLinkBackend(std::uint16_t targetNode = CSP::DEFAULT_COMM_NODE_ID,
                                      OBC::CSP::ICspRuntime& runtime = OBC::CSP::defaultRuntime(),
                                      GroundLinkHealthSemantics healthSemantics =
                                          GroundLinkHealthSemantics::DISABLED,
                                      CommCspDownlinkPolicy downlinkPolicy = CommCspDownlinkPolicy::AUTO);

    bool start() override;

    void stop() override;

    GroundLinkReceiveStatus receive(std::string& outChunk, std::uint32_t timeoutMs) override;

    GroundLinkSendStatus send(const std::uint8_t* data, std::size_t size) override;

    GroundLinkStats getStats() const override;

    GroundLinkObservationState getObservationState() const override;

    bool observeHealth() override;

  private:
    bool refreshStatusLocked_(std::uint32_t timeoutMs);

    std::uint16_t nextSeqLocked_();

    std::uint16_t nextStreamIdLocked_();

    bool probeDownlinkV3Locked_(std::uint32_t timeoutMs);

    bool ensureDownlinkV3ReadyForSendLocked_();

    bool refreshDownlinkV3StatusLocked_(std::uint32_t timeoutMs);

    bool probeDownlinkV2Locked_(std::uint32_t timeoutMs);

    bool refreshDownlinkV2StatusLocked_(std::uint32_t timeoutMs);

    GroundLinkSendStatus sendV1Locked_(const std::uint8_t* data, std::size_t size);

    GroundLinkSendStatus sendV3Locked_(const std::uint8_t* data, std::size_t size);

    GroundLinkSendStatus sendV2Locked_(const std::uint8_t* data, std::size_t size);

    void updateDownlinkV3StatsLocked_(const CSP::DownlinkControlV3Reply& reply);

    bool abortDownlinkV3Locked_(std::uint16_t streamId);

    void updateDownlinkV2StatsLocked_(const CSP::DownlinkStatusV2Reply& reply);

    bool abortDownlinkV2Locked_(std::uint16_t streamId);

    std::uint16_t m_targetNode;
    OBC::CSP::ICspRuntime& m_runtime;
    GroundLinkHealthSemantics m_healthSemantics;
    CommCspDownlinkPolicy m_downlinkPolicy;
    mutable std::mutex m_mutex;
    GroundLinkStats m_stats;
    std::uint32_t m_successfulStatusObservations;
    std::uint16_t m_seq;
    std::uint16_t m_streamId;
    bool m_downlinkV3ProbeCompleted;
    std::uint8_t m_downlinkV3ProbeAttempts;
    bool m_downlinkV3Enabled;
    bool m_downlinkV3FallbackLogged;
    bool m_downlinkV2ProbeCompleted;
    std::uint8_t m_downlinkV2ProbeAttempts;
    bool m_downlinkV2Enabled;
    bool m_downlinkV2FallbackLogged;
};

std::unique_ptr<IGroundLinkBackend> makeDirectTcpGroundLinkBackend(const std::string& host,
                                                                   std::uint16_t port,
                                                                   std::size_t maxChunkBytes = 512U);

std::unique_ptr<IGroundLinkBackend> makeCommCspGroundLinkBackend(std::uint16_t targetNode = CSP::DEFAULT_COMM_NODE_ID,
                                                                 OBC::CSP::ICspRuntime& runtime = OBC::CSP::defaultRuntime(),
                                                                 GroundLinkHealthSemantics healthSemantics =
                                                                     GroundLinkHealthSemantics::DISABLED);

}  // namespace COMM
}  // namespace OBC

#endif
