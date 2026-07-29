#ifndef OBC_SIMULATORS_EPS_EPSTRANSPORT_HPP
#define OBC_SIMULATORS_EPS_EPSTRANSPORT_HPP

#include <cstdint>
#include <memory>

#include "simulators/csp/CspRuntime.hpp"
#include "simulators/eps/EpsCspProtocol.hpp"

namespace OBC {
namespace EPS {

enum class TransportStatus : std::uint8_t {
    OK = 0U,
    TIMEOUT = 1U,
    INVALID_RESPONSE = 2U,
    INVALID_REQUEST = 3U,
    REMOTE_ERROR = 4U,
    TRANSPORT_ERROR = 5U,
};

class IEpsTransport {
  public:
    virtual ~IEpsTransport() = default;

    virtual TransportStatus getStatus(StatusData& outStatus) = 0;
    virtual TransportStatus setPdu(std::uint8_t channel, bool enabled, StatusData& outStatus) = 0;
    virtual TransportStatus setHeater(bool enabled, StatusData& outStatus) = 0;
    virtual TransportStatus reset(StatusData& outStatus) = 0;
};

class CspEpsTransport final : public IEpsTransport {
  public:
    explicit CspEpsTransport(std::uint16_t targetNode = CSP::DEFAULT_EPS_NODE_ID,
                             std::uint32_t timeoutMs = 200U,
                             ::OBC::CSP::ICspRuntime& runtime = ::OBC::CSP::defaultRuntime());

    TransportStatus getStatus(StatusData& outStatus) override;

    TransportStatus setPdu(std::uint8_t channel, bool enabled, StatusData& outStatus) override;

    TransportStatus setHeater(bool enabled, StatusData& outStatus) override;

    TransportStatus reset(StatusData& outStatus) override;

  private:
    bool ensureRuntime_();

    std::uint16_t nextSeq_();

    TransportStatus request_(const CSP::Request& request, CSP::ServicePort expectedService, StatusData& outStatus);

  private:
    std::uint16_t m_targetNode;
    std::uint32_t m_timeoutMs;
    std::uint16_t m_seq;
    ::OBC::CSP::ICspRuntime& m_runtime;
    bool m_runtimeReady;
};

std::uint16_t defaultEpsNodeIdFromEnvironment();
TransportStatus decodeEpsReply(const CSP::Request& request,
                               CSP::ServicePort expectedService,
                               const void* replyData,
                               std::size_t replySize,
                               StatusData& outStatus);

std::unique_ptr<IEpsTransport> makeDefaultEpsTransport(std::uint32_t timeoutMs = 200U);
std::unique_ptr<IEpsTransport> makeDefaultEpsTransport(::OBC::CSP::ICspRuntime& runtime, std::uint32_t timeoutMs = 200U);

}  // namespace EPS
}  // namespace OBC

#endif
