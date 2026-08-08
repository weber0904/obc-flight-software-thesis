#ifndef OBC_SIMULATORS_COMM_COMMSIMMODEL_HPP
#define OBC_SIMULATORS_COMM_COMMSIMMODEL_HPP

#include <cstddef>
#include <cstdint>
#include <deque>

#include "simulators/comm/CommCspProtocol.hpp"

namespace OBC {
namespace COMM {

struct CommSimConfig {
    std::size_t uplinkQueueCapacity = 4096U;
};

struct CommSimStatus {
    bool physicalLinkConnected = false;
    bool forcedDisconnected = false;
    bool downlinkBackpressure = false;
    bool injectedIoError = false;
    bool effectiveLinkConnected = false;
    std::size_t uplinkQueueDepth = 0U;
    std::size_t uplinkQueueCapacity = 0U;
    std::uint32_t rxChunks = 0U;
    std::uint32_t txChunks = 0U;
    std::uint32_t rxErrors = 0U;
    std::uint32_t txErrors = 0U;
    std::uint32_t uplinkDroppedChunks = 0U;
    std::uint32_t uplinkDroppedBytes = 0U;
};

class CommSimModel {
  public:
    explicit CommSimModel(const CommSimConfig& config = CommSimConfig{});

    void reset();

    void setPhysicalLinkConnected(bool connected);

    void setForcedDisconnected(bool forcedDisconnected);

    void setDownlinkBackpressure(bool enabled);

    void setInjectedIoError(bool enabled);

    bool ingestUplinkBytes(const std::uint8_t* data, std::size_t size);

    void recordRxError();

    CSP::ChunkReply handleUplinkPoll(const CSP::UplinkPollRequest& request);

    CSP::ChunkReply validateDownlinkWriteRequest(const CSP::DownlinkWriteRequest& request) const;

    bool shouldAttemptSerialDownlink() const;

    CSP::ChunkReply beginDownlinkWrite(const CSP::DownlinkWriteRequest& request, bool& shouldWrite);

    void completeDownlinkWrite(bool success);

    CSP::LinkStatusReply handleLinkStatus(const CSP::LinkStatusRequest& request) const;

    CommSimStatus status() const;

    std::uint8_t linkFlags() const;

  private:
    bool isLinkAvailable_() const;

    static bool validHeader_(const CSP::RequestHeader& header, CSP::ServicePort service);

    CommSimConfig m_config;
    bool m_physicalLinkConnected;
    bool m_forcedDisconnected;
    bool m_downlinkBackpressure;
    bool m_injectedIoError;
    std::deque<std::uint8_t> m_uplinkQueue;
    std::uint32_t m_rxChunks;
    std::uint32_t m_txChunks;
    std::uint32_t m_rxErrors;
    std::uint32_t m_txErrors;
    std::uint32_t m_uplinkDroppedChunks;
    std::uint32_t m_uplinkDroppedBytes;
};

}  // namespace COMM
}  // namespace OBC

#endif
