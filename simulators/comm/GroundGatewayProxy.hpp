#ifndef OBC_SIMULATORS_COMM_GROUNDGATEWAYPROXY_HPP
#define OBC_SIMULATORS_COMM_GROUNDGATEWAYPROXY_HPP

#include <atomic>
#include <cstdint>
#include <string>

namespace OBC {
namespace COMM {

enum class GroundGatewaySouthboundMode {
    SERIAL,
    TCP_CLIENT,
};

struct GroundGatewayConfig {
    std::string gdsHost = "127.0.0.1";
    std::uint16_t gdsPort = 50000U;
    GroundGatewaySouthboundMode southboundMode = GroundGatewaySouthboundMode::SERIAL;
    std::string serialDevice;
    std::string rfTcpHost = "127.0.0.1";
    std::uint16_t rfTcpPort = 0U;
    std::string linkIdentity = "generic";
    std::uint32_t baudrate = 115200U;
    std::uint32_t serialTxPreambleLines = 0U;
    std::uint32_t serialTxPreambleDelayMs = 0U;
    std::string captureGdsToSouthboundPath;
    std::string captureSouthboundToGdsPath;
};

class GroundGatewayProxy {
  public:
    explicit GroundGatewayProxy(const GroundGatewayConfig& config);

    ~GroundGatewayProxy();

    bool start();

    void run();

    void stop();

  private:
    bool ensureGdsConnected_();

    bool ensureSouthboundOpen_();

    bool sendSerialPreamble_();

    void closeLinks_();

    bool openCaptureFiles_();

    void closeCaptureFiles_();

    bool captureBytes_(int& fd, const char* direction, const std::uint8_t* data, std::size_t size);

    bool pumpGdsToSerial_();

    bool pumpSerialToGds_();

  private:
    GroundGatewayConfig m_config;
    std::atomic<bool> m_running;
    int m_gdsFd;
    int m_southboundFd;
    int m_captureGdsToSouthboundFd;
    int m_captureSouthboundToGdsFd;
};

}  // namespace COMM
}  // namespace OBC

#endif
