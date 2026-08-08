#ifndef OBC_SIMULATORS_ADCS_ADCSTRANSPORT_HPP
#define OBC_SIMULATORS_ADCS_ADCSTRANSPORT_HPP

#include <cstdint>
#include <memory>

#include "simulators/adcs/AdcsCspProtocol.hpp"
#include "simulators/csp/CspRuntime.hpp"

namespace OBC {
namespace ADCS {

enum class TransportStatus : std::uint8_t {
    OK = 0U,
    TIMEOUT = 1U,
    INVALID_RESPONSE = 2U,
    INVALID_REQUEST = 3U,
    REMOTE_ERROR = 4U,
    TRANSPORT_ERROR = 5U,
};

class IAdcsTransport {
  public:
    virtual ~IAdcsTransport() = default;

    virtual TransportStatus getState(StateData& outState) = 0;
    virtual TransportStatus setMode(std::uint8_t mode, StateData& outState) = 0;
    virtual TransportStatus setTarget(double q0, double q1, double q2, double q3, StateData& outState) = 0;
    virtual TransportStatus calibrate(std::uint8_t sensorId, StateData& outState) = 0;
    virtual TransportStatus reset(StateData& outState) = 0;
};

class CspAdcsTransport final : public IAdcsTransport {
  public:
    explicit CspAdcsTransport(std::uint16_t targetNode = CSP::DEFAULT_ADCS_NODE_ID,
                              std::uint32_t timeoutMs = 200U,
                              ::OBC::CSP::ICspRuntime& runtime = ::OBC::CSP::defaultRuntime());

    TransportStatus getState(StateData& outState) override;

    TransportStatus setMode(std::uint8_t mode, StateData& outState) override;

    TransportStatus setTarget(double q0, double q1, double q2, double q3, StateData& outState) override;

    TransportStatus calibrate(std::uint8_t sensorId, StateData& outState) override;

    TransportStatus reset(StateData& outState) override;

  private:
    bool ensureRuntime_();

    std::uint16_t nextSeq_();

    TransportStatus request_(const CSP::Request& request, CSP::ServicePort expectedService, StateData& outState);

  private:
    std::uint16_t m_targetNode;
    std::uint32_t m_timeoutMs;
    std::uint16_t m_seq;
    ::OBC::CSP::ICspRuntime& m_runtime;
    bool m_runtimeReady;
};

std::uint16_t defaultAdcsNodeIdFromEnvironment();
TransportStatus decodeAdcsReply(const CSP::Request& request,
                                CSP::ServicePort expectedService,
                                const void* replyData,
                                std::size_t replySize,
                                StateData& outState);

std::unique_ptr<IAdcsTransport> makeDefaultAdcsTransport(std::uint32_t timeoutMs = 200U);
std::unique_ptr<IAdcsTransport> makeDefaultAdcsTransport(::OBC::CSP::ICspRuntime& runtime, std::uint32_t timeoutMs = 200U);

}  // namespace ADCS
}  // namespace OBC

#endif
