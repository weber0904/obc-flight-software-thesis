#include "simulators/comm/RadioTransport.hpp"

#include <sstream>

namespace OBC {
namespace COMM {

namespace {

RadioTransportStatus mapStatus(ByteStreamStatus status) {
    switch (status) {
        case ByteStreamStatus::OK:
            return RadioTransportStatus::OK;
        case ByteStreamStatus::TIMEOUT:
            return RadioTransportStatus::TIMEOUT;
        case ByteStreamStatus::IO_ERROR:
        default:
            return RadioTransportStatus::TRANSPORT_ERROR;
    }
}

bool parseStatusLine(const std::string& line, RadioStatus& status) {
    std::istringstream stream(line);
    std::string token;
    if (!(stream >> token)) {
        return false;
    }
    if (token != "OK" && token != "STATUS") {
        return false;
    }

    bool haveEnabled = false;
    bool havePower = false;
    bool haveFreq = false;
    bool haveTemp = false;
    bool haveRssi = false;

    while (stream >> token) {
        const std::size_t sep = token.find('=');
        if (sep == std::string::npos) {
            return false;
        }

        const std::string key = token.substr(0, sep);
        const std::string value = token.substr(sep + 1);

        try {
            if (key == "enabled") {
                status.enabled = std::stoi(value) != 0;
                haveEnabled = true;
            } else if (key == "power") {
                status.powerDbm = static_cast<std::uint8_t>(std::stoul(value));
                havePower = true;
            } else if (key == "freq") {
                status.freqHz = static_cast<std::uint32_t>(std::stoul(value));
                haveFreq = true;
            } else if (key == "temp") {
                status.temperatureC = std::stof(value);
                haveTemp = true;
            } else if (key == "rssi") {
                status.rssiDbm = static_cast<std::int16_t>(std::stoi(value));
                haveRssi = true;
            } else {
                return false;
            }
        } catch (...) {
            return false;
        }
    }

    return haveEnabled && havePower && haveFreq && haveTemp && haveRssi;
}

class MockTextRadioProtocolAdapter final : public IRadioProtocolAdapter {
  public:
    const char* name() const override {
        return "mock-text";
    }

    RadioTransportStatus getStatus(IByteStreamTransport& link, RadioStatus& status) const override {
        return this->requestAndParse_(link, "STATUS\n", status);
    }

    RadioTransportStatus setEnabled(IByteStreamTransport& link, bool enabled, RadioStatus& status) const override {
        return this->requestAndParse_(link, enabled ? "ENABLE 1\n" : "ENABLE 0\n", status);
    }

    RadioTransportStatus setPower(IByteStreamTransport& link, std::uint8_t powerDbm, RadioStatus& status) const override {
        std::ostringstream request;
        request << "POWER " << static_cast<unsigned int>(powerDbm) << "\n";
        return this->requestAndParse_(link, request.str(), status);
    }

    RadioTransportStatus setFrequency(IByteStreamTransport& link,
                                      std::uint32_t freqHz,
                                      RadioStatus& status) const override {
        std::ostringstream request;
        request << "FREQ " << freqHz << "\n";
        return this->requestAndParse_(link, request.str(), status);
    }

  private:
    RadioTransportStatus requestAndParse_(IByteStreamTransport& link,
                                          const std::string& request,
                                          RadioStatus& status) const {
        std::string response;
        const ByteStreamStatus streamStatus = link.exchange(request, response);
        if (streamStatus != ByteStreamStatus::OK) {
            return mapStatus(streamStatus);
        }

        if (!parseStatusLine(response, status)) {
            return RadioTransportStatus::INVALID_RESPONSE;
        }

        return RadioTransportStatus::OK;
    }
};

class TransparentPassiveRadioProtocolAdapter final : public IRadioProtocolAdapter {
  public:
    const char* name() const override {
        return "transparent-passive";
    }

    RadioTransportStatus getStatus(IByteStreamTransport& link, RadioStatus& status) const override {
        static_cast<void>(link);
        static_cast<void>(status);
        return RadioTransportStatus::UNSUPPORTED;
    }

    RadioTransportStatus setEnabled(IByteStreamTransport& link, bool enabled, RadioStatus& status) const override {
        static_cast<void>(link);
        static_cast<void>(enabled);
        static_cast<void>(status);
        return RadioTransportStatus::UNSUPPORTED;
    }

    RadioTransportStatus setPower(IByteStreamTransport& link,
                                  std::uint8_t powerDbm,
                                  RadioStatus& status) const override {
        static_cast<void>(link);
        static_cast<void>(powerDbm);
        static_cast<void>(status);
        return RadioTransportStatus::UNSUPPORTED;
    }

    RadioTransportStatus setFrequency(IByteStreamTransport& link,
                                      std::uint32_t freqHz,
                                      RadioStatus& status) const override {
        static_cast<void>(link);
        static_cast<void>(freqHz);
        static_cast<void>(status);
        return RadioTransportStatus::UNSUPPORTED;
    }
};

}  // namespace

HostedRadioTransport::HostedRadioTransport(std::shared_ptr<IByteStreamTransport> link,
                                           std::unique_ptr<IRadioProtocolAdapter> protocol)
    : m_link(std::move(link)), m_protocol(std::move(protocol)) {}

HostedRadioTransport::~HostedRadioTransport() = default;

RadioTransportStatus HostedRadioTransport::getStatus(RadioStatus& status) {
    if (!this->m_link || !this->m_protocol) {
        return RadioTransportStatus::TRANSPORT_ERROR;
    }
    return this->m_protocol->getStatus(*this->m_link, status);
}

RadioTransportStatus HostedRadioTransport::setEnabled(bool enabled, RadioStatus& status) {
    if (!this->m_link || !this->m_protocol) {
        return RadioTransportStatus::TRANSPORT_ERROR;
    }
    return this->m_protocol->setEnabled(*this->m_link, enabled, status);
}

RadioTransportStatus HostedRadioTransport::setPower(std::uint8_t powerDbm, RadioStatus& status) {
    if (!this->m_link || !this->m_protocol) {
        return RadioTransportStatus::TRANSPORT_ERROR;
    }
    return this->m_protocol->setPower(*this->m_link, powerDbm, status);
}

RadioTransportStatus HostedRadioTransport::setFrequency(std::uint32_t freqHz, RadioStatus& status) {
    if (!this->m_link || !this->m_protocol) {
        return RadioTransportStatus::TRANSPORT_ERROR;
    }
    return this->m_protocol->setFrequency(*this->m_link, freqHz, status);
}

ByteStreamStats HostedRadioTransport::getLinkStats() const {
    return this->m_link ? this->m_link->getStats() : ByteStreamStats{0U, 0U, 0U, 0U, false};
}

bool isSupportedRadioProtocol(const std::string& name) {
    return name == "mock-text" || name == "transparent-passive";
}

std::unique_ptr<IRadioTransport> makeTcpMockRadioTransport(const std::string& host,
                                                           std::uint16_t port,
                                                           std::uint32_t timeoutMs,
                                                           const std::string& protocolName) {
    return makeSharedMockRadioTransport(std::shared_ptr<IByteStreamTransport>(
        new TcpByteStreamTransport(host, port, timeoutMs)),
        protocolName);
}

std::unique_ptr<IRadioProtocolAdapter> makeRadioProtocolAdapter(const std::string& name) {
    if (name == "mock-text") {
        return std::unique_ptr<IRadioProtocolAdapter>(new MockTextRadioProtocolAdapter());
    }
    if (name == "transparent-passive") {
        return std::unique_ptr<IRadioProtocolAdapter>(new TransparentPassiveRadioProtocolAdapter());
    }
    return nullptr;
}

std::unique_ptr<IRadioTransport> makeSharedMockRadioTransport(const std::shared_ptr<IByteStreamTransport>& link,
                                                              const std::string& protocolName) {
    std::unique_ptr<IRadioProtocolAdapter> protocol = makeRadioProtocolAdapter(protocolName);
    if (!protocol) {
        return nullptr;
    }
    return std::unique_ptr<IRadioTransport>(new HostedRadioTransport(link, std::move(protocol)));
}

std::unique_ptr<IRadioTransport> makeSerialDeviceRadioTransport(const std::string& devicePath,
                                                                std::uint32_t timeoutMs,
                                                                const std::string& protocolName) {
    return makeSharedMockRadioTransport(std::shared_ptr<IByteStreamTransport>(
        new SerialByteStreamTransport(devicePath, timeoutMs)),
        protocolName);
}

std::unique_ptr<IRadioTransport> makePtyRadioTransport(const std::string& devicePath,
                                                       std::uint32_t timeoutMs,
                                                       const std::string& protocolName) {
    return makeSerialDeviceRadioTransport(devicePath, timeoutMs, protocolName);
}

}  // namespace COMM
}  // namespace OBC
