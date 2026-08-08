#include "simulators/comm/GroundGatewayProxy.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <unistd.h>

#include "simulators/comm/StreamIo.hpp"

namespace OBC {
namespace COMM {

namespace {

static constexpr std::uint32_t IO_POLL_TIMEOUT_MS = 20U;
static constexpr std::uint32_t PREAMBLE_STOP_POLL_MS = 50U;

bool gatewayByteTraceEnabled() {
    const char* value = std::getenv("COMM_GATEWAY_BYTE_TRACE");
    return value != nullptr && std::strcmp(value, "0") != 0;
}

std::string hexPreview(const std::uint8_t* data, const std::size_t size, const std::size_t maxBytes = 64U) {
    if (data == nullptr || size == 0U) {
        return "";
    }
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    const std::size_t previewBytes = std::min(size, maxBytes);
    for (std::size_t index = 0U; index < previewBytes; ++index) {
        if (index != 0U) {
            stream << ' ';
        }
        stream << std::setw(2) << static_cast<unsigned int>(data[index]);
    }
    if (size > previewBytes) {
        stream << " ...";
    }
    return stream.str();
}

}  // namespace

GroundGatewayProxy::GroundGatewayProxy(const GroundGatewayConfig& config)
    : m_config(config),
      m_running(false),
      m_gdsFd(-1),
      m_southboundFd(-1),
      m_captureGdsToSouthboundFd(-1),
      m_captureSouthboundToGdsFd(-1) {}

GroundGatewayProxy::~GroundGatewayProxy() {
    this->stop();
}

bool GroundGatewayProxy::start() {
    if (this->m_running.load()) {
        return true;
    }

    if (!this->openCaptureFiles_()) {
        this->closeCaptureFiles_();
        return false;
    }
    this->m_running.store(true);
    return true;
}

void GroundGatewayProxy::run() {
    if (!this->start()) {
        return;
    }

    while (this->m_running.load()) {
        const bool gdsReady = this->ensureGdsConnected_();
        if (!gdsReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        const bool southboundReady = this->ensureSouthboundOpen_();
        if (!southboundReady) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        const bool movedGds = this->pumpGdsToSerial_();
        const bool movedSerial = this->pumpSerialToGds_();
        if (!movedGds && !movedSerial) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    this->closeLinks_();
}

void GroundGatewayProxy::stop() {
    this->m_running.store(false);
    this->closeLinks_();
    this->closeCaptureFiles_();
}

bool GroundGatewayProxy::ensureGdsConnected_() {
    if (this->m_gdsFd >= 0) {
        return true;
    }
    const bool connected = openTcpClient(this->m_config.gdsHost, this->m_config.gdsPort, this->m_gdsFd);
    if (connected) {
        std::cerr << "ground_ttc_gateway gds-connected host=" << this->m_config.gdsHost
                  << " port=" << this->m_config.gdsPort << "\n";
    }
    return connected;
}

bool GroundGatewayProxy::ensureSouthboundOpen_() {
    if (this->m_southboundFd >= 0) {
        return true;
    }
    if (this->m_config.southboundMode == GroundGatewaySouthboundMode::TCP_CLIENT) {
        const bool opened = openTcpClient(this->m_config.rfTcpHost, this->m_config.rfTcpPort, this->m_southboundFd);
        if (opened) {
            std::cerr << "ground_ttc_gateway southbound-opened mode=tcp-client host=" << this->m_config.rfTcpHost
                      << " port=" << this->m_config.rfTcpPort << "\n";
        }
        return opened;
    } else {
        if (!openRawSerial(this->m_config.serialDevice, this->m_config.baudrate, this->m_southboundFd)) {
            std::cerr << "ground_ttc_gateway southbound-open-failed mode=serial device="
                      << this->m_config.serialDevice << " baudrate=" << this->m_config.baudrate << "\n";
            return false;
        }
        std::cerr << "ground_ttc_gateway southbound-opened mode=serial device=" << this->m_config.serialDevice
                  << " baudrate=" << this->m_config.baudrate << "\n";
        if (!this->sendSerialPreamble_()) {
            std::cerr << "ground_ttc_gateway southbound-preamble-failed mode=serial device="
                      << this->m_config.serialDevice << "\n";
            this->closeLinks_();
            return false;
        }
    }
    return true;
}

bool GroundGatewayProxy::sendSerialPreamble_() {
    if (this->m_config.serialTxPreambleLines == 0U) {
        return true;
    }

    for (std::uint32_t i = 0U; i < this->m_config.serialTxPreambleLines; ++i) {
        if (!this->m_running.load()) {
            return false;
        }

        const std::string line = "COMM-GATEWAY-PREAMBLE-" + std::to_string(i) + "\n";
        if (!writeAll(this->m_southboundFd,
                      reinterpret_cast<const std::uint8_t*>(line.data()),
                      line.size())) {
            return false;
        }
        std::uint32_t remainingDelayMs = this->m_config.serialTxPreambleDelayMs;
        while (remainingDelayMs > 0U) {
            if (!this->m_running.load()) {
                return false;
            }
            const std::uint32_t delayMs =
                remainingDelayMs < PREAMBLE_STOP_POLL_MS ? remainingDelayMs : PREAMBLE_STOP_POLL_MS;
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            remainingDelayMs -= delayMs;
        }
    }
    return true;
}

void GroundGatewayProxy::closeLinks_() {
    closeFd(this->m_gdsFd);
    closeFd(this->m_southboundFd);
}

bool GroundGatewayProxy::openCaptureFiles_() {
    if (this->m_captureGdsToSouthboundFd < 0 && !this->m_config.captureGdsToSouthboundPath.empty()) {
        this->m_captureGdsToSouthboundFd =
            ::open(this->m_config.captureGdsToSouthboundPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (this->m_captureGdsToSouthboundFd < 0) {
            return false;
        }
    }
    if (this->m_captureSouthboundToGdsFd < 0 && !this->m_config.captureSouthboundToGdsPath.empty()) {
        this->m_captureSouthboundToGdsFd =
            ::open(this->m_config.captureSouthboundToGdsPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (this->m_captureSouthboundToGdsFd < 0) {
            return false;
        }
    }
    return true;
}

void GroundGatewayProxy::closeCaptureFiles_() {
    closeFd(this->m_captureGdsToSouthboundFd);
    closeFd(this->m_captureSouthboundToGdsFd);
}

bool GroundGatewayProxy::captureBytes_(int& fd, const char* direction, const std::uint8_t* data, std::size_t size) {
    if (fd < 0 || size == 0U) {
        return true;
    }
    if (writeAll(fd, data, size)) {
        return true;
    }
    std::cerr << "ground_ttc_gateway capture-write-error direction=" << direction << "\n";
    closeFd(fd);
    return false;
}

bool GroundGatewayProxy::pumpGdsToSerial_() {
    std::uint8_t buffer[512] = {};
    std::size_t bytesRead = 0U;
    const StreamReadStatus status = readSome(this->m_gdsFd, IO_POLL_TIMEOUT_MS, buffer, sizeof(buffer), bytesRead);
    if (status == StreamReadStatus::TIMEOUT) {
        return false;
    }
    if (status == StreamReadStatus::CLOSED || status == StreamReadStatus::IO_ERROR) {
        std::cerr << "ground_ttc_gateway gds-read-failed status="
                  << (status == StreamReadStatus::CLOSED ? "CLOSED" : "IO_ERROR") << "\n";
        this->closeLinks_();
        return false;
    }
    if (bytesRead != 0U) {
        std::cerr << "ground_ttc_gateway gds-read bytes=" << bytesRead << "\n";
        if (gatewayByteTraceEnabled()) {
            std::cerr << "ground_ttc_gateway gds-read-hex bytes=" << bytesRead
                      << " preview=" << hexPreview(buffer, bytesRead, bytesRead) << "\n";
            std::cerr << "ground_ttc_gateway southbound-write-hex bytes=" << bytesRead
                      << " preview=" << hexPreview(buffer, bytesRead, bytesRead) << "\n";
        }
    }
    (void)this->captureBytes_(this->m_captureGdsToSouthboundFd, "gds-to-southbound", buffer, bytesRead);
    if (!writeAll(this->m_southboundFd, buffer, bytesRead)) {
        std::cerr << "ground_ttc_gateway southbound-write-failed bytes=" << bytesRead << "\n";
        this->closeLinks_();
        return false;
    }
    return bytesRead != 0U;
}

bool GroundGatewayProxy::pumpSerialToGds_() {
    std::uint8_t buffer[512] = {};
    std::size_t bytesRead = 0U;
    const StreamReadStatus status =
        readSome(this->m_southboundFd, IO_POLL_TIMEOUT_MS, buffer, sizeof(buffer), bytesRead);
    if (status == StreamReadStatus::TIMEOUT) {
        return false;
    }
    if (status == StreamReadStatus::CLOSED || status == StreamReadStatus::IO_ERROR) {
        std::cerr << "ground_ttc_gateway southbound-read-failed status="
                  << (status == StreamReadStatus::CLOSED ? "CLOSED" : "IO_ERROR") << "\n";
        this->closeLinks_();
        return false;
    }
    if (bytesRead != 0U) {
        std::cerr << "ground_ttc_gateway southbound-read bytes=" << bytesRead << "\n";
        if (gatewayByteTraceEnabled()) {
            std::cerr << "ground_ttc_gateway southbound-read-hex bytes=" << bytesRead
                      << " preview=" << hexPreview(buffer, bytesRead, bytesRead) << "\n";
        }
    }
    (void)this->captureBytes_(this->m_captureSouthboundToGdsFd, "southbound-to-gds", buffer, bytesRead);
    if (!writeAll(this->m_gdsFd, buffer, bytesRead)) {
        std::cerr << "ground_ttc_gateway gds-write-failed bytes=" << bytesRead << "\n";
        this->closeLinks_();
        return false;
    }
    return bytesRead != 0U;
}

}  // namespace COMM
}  // namespace OBC
