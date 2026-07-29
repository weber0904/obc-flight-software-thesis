#ifndef OBC_COMM_RadioTransport_HPP
#define OBC_COMM_RadioTransport_HPP

#include <cstdint>
#include <memory>
#include <string>

#include "simulators/comm/ByteStreamTransport.hpp"

namespace OBC {
namespace COMM {

enum class RadioTransportStatus {
    OK,
    TIMEOUT,
    TRANSPORT_ERROR,
    INVALID_RESPONSE,
    UNSUPPORTED
};

struct RadioStatus {
    bool enabled;
    std::uint8_t powerDbm;
    std::uint32_t freqHz;
    float temperatureC;
    std::int16_t rssiDbm;
};

class IRadioTransport {
  public:
    virtual ~IRadioTransport() = default;

    virtual RadioTransportStatus getStatus(RadioStatus& status) = 0;

    virtual RadioTransportStatus setEnabled(bool enabled, RadioStatus& status) = 0;

    virtual RadioTransportStatus setPower(std::uint8_t powerDbm, RadioStatus& status) = 0;

    virtual RadioTransportStatus setFrequency(std::uint32_t freqHz, RadioStatus& status) = 0;

    virtual ByteStreamStats getLinkStats() const = 0;
};

class IRadioProtocolAdapter {
  public:
    virtual ~IRadioProtocolAdapter() = default;

    virtual const char* name() const = 0;

    virtual RadioTransportStatus getStatus(IByteStreamTransport& link, RadioStatus& status) const = 0;

    virtual RadioTransportStatus setEnabled(IByteStreamTransport& link, bool enabled, RadioStatus& status) const = 0;

    virtual RadioTransportStatus setPower(IByteStreamTransport& link, std::uint8_t powerDbm, RadioStatus& status) const = 0;

    virtual RadioTransportStatus setFrequency(IByteStreamTransport& link,
                                              std::uint32_t freqHz,
                                              RadioStatus& status) const = 0;
};

class HostedRadioTransport final : public IRadioTransport {
  public:
    HostedRadioTransport(std::shared_ptr<IByteStreamTransport> link, std::unique_ptr<IRadioProtocolAdapter> protocol);

    ~HostedRadioTransport() override;

    RadioTransportStatus getStatus(RadioStatus& status) override;

    RadioTransportStatus setEnabled(bool enabled, RadioStatus& status) override;

    RadioTransportStatus setPower(std::uint8_t powerDbm, RadioStatus& status) override;

    RadioTransportStatus setFrequency(std::uint32_t freqHz, RadioStatus& status) override;

    ByteStreamStats getLinkStats() const override;

  private:
    std::shared_ptr<IByteStreamTransport> m_link;
    std::unique_ptr<IRadioProtocolAdapter> m_protocol;
};

bool isSupportedRadioProtocol(const std::string& name);

std::unique_ptr<IRadioProtocolAdapter> makeRadioProtocolAdapter(const std::string& name);

std::unique_ptr<IRadioTransport> makeTcpMockRadioTransport(const std::string& host,
                                                           std::uint16_t port,
                                                           std::uint32_t timeoutMs = 200U,
                                                           const std::string& protocolName = "mock-text");

std::unique_ptr<IRadioTransport> makeSharedMockRadioTransport(const std::shared_ptr<IByteStreamTransport>& link,
                                                              const std::string& protocolName = "mock-text");

std::unique_ptr<IRadioTransport> makeSerialDeviceRadioTransport(const std::string& devicePath,
                                                                std::uint32_t timeoutMs = 200U,
                                                                const std::string& protocolName = "mock-text");

std::unique_ptr<IRadioTransport> makePtyRadioTransport(const std::string& devicePath,
                                                       std::uint32_t timeoutMs = 200U,
                                                       const std::string& protocolName = "mock-text");

}  // namespace COMM
}  // namespace OBC

#endif
