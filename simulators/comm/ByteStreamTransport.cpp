#include "simulators/comm/ByteStreamTransport.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

namespace OBC {
namespace COMM {

namespace {

void closeFd(int& fd) {
    if (fd >= 0) {
        (void)::close(fd);
        fd = -1;
    }
}

bool lookupBaudrate(std::uint32_t baudrate, speed_t& speed) {
    switch (baudrate) {
        case 9600U:
            speed = B9600;
            return true;
        case 19200U:
            speed = B19200;
            return true;
        case 38400U:
            speed = B38400;
            return true;
        case 57600U:
            speed = B57600;
            return true;
        case 115200U:
            speed = B115200;
            return true;
#ifdef B230400
        case 230400U:
            speed = B230400;
            return true;
#endif
#ifdef B460800
        case 460800U:
            speed = B460800;
            return true;
#endif
        default:
            return false;
    }
}

bool configureRawFd(int fd, std::uint32_t baudrate) {
    struct termios tty;
    speed_t speed = B115200;
    if (::tcgetattr(fd, &tty) != 0) {
        return false;
    }
    if (!lookupBaudrate(baudrate, speed)) {
        return false;
    }
    ::cfmakeraw(&tty);
    if (::cfsetispeed(&tty, speed) != 0 || ::cfsetospeed(&tty, speed) != 0) {
        return false;
    }
    if (::tcsetattr(fd, TCSANOW, &tty) != 0) {
        return false;
    }
    return ::tcflush(fd, TCIOFLUSH) == 0;
}

ByteStreamStatus writeAll(int fd, const std::uint8_t* payload, std::size_t size, std::uint32_t timeoutMs) {
    std::size_t written = 0;
    while (written < size) {
        struct pollfd pfd;
        std::memset(&pfd, 0, sizeof(pfd));
        pfd.fd = fd;
        pfd.events = POLLOUT;

        const int pollResult = ::poll(&pfd, 1, static_cast<int>(timeoutMs));
        if (pollResult == 0) {
            return ByteStreamStatus::TIMEOUT;
        }
        if (pollResult < 0) {
            if (errno == EINTR) {
                continue;
            }
            return ByteStreamStatus::IO_ERROR;
        }

        const ssize_t result = ::write(fd, payload + written, static_cast<std::size_t>(size - written));
        if (result <= 0) {
            if (errno == EINTR) {
                continue;
            }
            return ByteStreamStatus::IO_ERROR;
        }
        written += static_cast<std::size_t>(result);
    }
    return ByteStreamStatus::OK;
}

ByteStreamStatus writeAll(int fd, const std::string& payload, std::uint32_t timeoutMs) {
    return writeAll(fd, reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size(), timeoutMs);
}

ByteStreamStatus readDelimited(int fd, std::uint32_t timeoutMs, std::string& payload, char delimiter) {
    payload.clear();
    char ch = 0;

    while (true) {
        struct pollfd pfd;
        std::memset(&pfd, 0, sizeof(pfd));
        pfd.fd = fd;
        pfd.events = POLLIN;

        const int pollResult = ::poll(&pfd, 1, static_cast<int>(timeoutMs));
        if (pollResult == 0) {
            return ByteStreamStatus::TIMEOUT;
        }
        if (pollResult < 0) {
            if (errno == EINTR) {
                continue;
            }
            return ByteStreamStatus::IO_ERROR;
        }

        const ssize_t bytesRead = ::read(fd, &ch, 1);
        if (bytesRead <= 0) {
            if (bytesRead < 0 && errno == EINTR) {
                continue;
            }
            return ByteStreamStatus::IO_ERROR;
        }

        if (ch == delimiter) {
            return ByteStreamStatus::OK;
        }

        payload.push_back(ch);
    }
}

}  // namespace

TcpByteStreamTransport::TcpByteStreamTransport(const std::string& host, std::uint16_t port, std::uint32_t timeoutMs)
    : m_host(host), m_port(port), m_timeoutMs(timeoutMs), m_fd(-1), m_stats{0U, 0U, 0U, 0U, false} {}

TcpByteStreamTransport::~TcpByteStreamTransport() {
    this->disconnect();
}

bool TcpByteStreamTransport::connect() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->connectUnlocked_();
}

bool TcpByteStreamTransport::connectUnlocked_() {
    if (this->m_fd >= 0) {
        this->m_stats.connected = true;
        return true;
    }

    this->m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (this->m_fd < 0) {
        this->m_stats.txErrors += 1U;
        return false;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(this->m_port);
    if (::inet_pton(AF_INET, this->m_host.c_str(), &addr.sin_addr) != 1) {
        this->m_stats.txErrors += 1U;
        closeFd(this->m_fd);
        return false;
    }

    if (::connect(this->m_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
        this->m_stats.txErrors += 1U;
        closeFd(this->m_fd);
        return false;
    }

    this->m_stats.connected = true;
    return true;
}

void TcpByteStreamTransport::disconnect() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->disconnectUnlocked_();
}

void TcpByteStreamTransport::disconnectUnlocked_() {
    closeFd(this->m_fd);
    this->m_stats.connected = false;
}

bool TcpByteStreamTransport::isConnected() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_fd >= 0;
}

ByteStreamStatus TcpByteStreamTransport::exchange(const std::string& request, std::string& response) {
    return this->exchangeDelimited(request, response, '\n');
}

ByteStreamStatus TcpByteStreamTransport::exchangeDelimited(const std::string& request,
                                                           std::string& response,
                                                           char delimiter) {
    std::lock_guard<std::mutex> lock(this->m_mutex);

    if (this->m_fd < 0 && !this->connectUnlocked_()) {
        return ByteStreamStatus::IO_ERROR;
    }

    const ByteStreamStatus writeStatus = writeAll(this->m_fd, request, this->m_timeoutMs);
    if (writeStatus != ByteStreamStatus::OK) {
        this->m_stats.txErrors += 1U;
        this->disconnectUnlocked_();
        return writeStatus;
    }
    this->m_stats.txBytes += static_cast<std::uint32_t>(request.size());

    const ByteStreamStatus status = readDelimited(this->m_fd, this->m_timeoutMs, response, delimiter);
    if (status == ByteStreamStatus::OK) {
        this->m_stats.rxBytes += static_cast<std::uint32_t>(response.size() + 1U);
        return status;
    }

    if (status == ByteStreamStatus::TIMEOUT) {
        this->m_stats.rxErrors += 1U;
    } else {
        this->m_stats.rxErrors += 1U;
        this->disconnectUnlocked_();
    }
    return status;
}

ByteStreamStatus TcpByteStreamTransport::send(const std::uint8_t* data, std::size_t size) {
    std::lock_guard<std::mutex> lock(this->m_mutex);

    if (data == nullptr || size == 0U) {
        return ByteStreamStatus::IO_ERROR;
    }

    if (this->m_fd < 0 && !this->connectUnlocked_()) {
        return ByteStreamStatus::IO_ERROR;
    }

    const ByteStreamStatus writeStatus = writeAll(this->m_fd, data, size, this->m_timeoutMs);
    if (writeStatus != ByteStreamStatus::OK) {
        this->m_stats.txErrors += 1U;
        this->disconnectUnlocked_();
        return writeStatus;
    }
    this->m_stats.txBytes += static_cast<std::uint32_t>(size);
    return ByteStreamStatus::OK;
}

ByteStreamStats TcpByteStreamTransport::getStats() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_stats;
}

SerialByteStreamTransport::SerialByteStreamTransport(const std::string& devicePath,
                                                     std::uint32_t timeoutMs,
                                                     std::uint32_t baudrate)
    : m_devicePath(devicePath),
      m_timeoutMs(timeoutMs),
      m_baudrate(baudrate),
      m_fd(-1),
      m_stats{0U, 0U, 0U, 0U, false} {}

SerialByteStreamTransport::~SerialByteStreamTransport() {
    this->disconnect();
}

bool SerialByteStreamTransport::connect() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->connectUnlocked_();
}

bool SerialByteStreamTransport::connectUnlocked_() {
    if (this->m_fd >= 0) {
        this->m_stats.connected = true;
        return true;
    }

    this->m_fd = ::open(this->m_devicePath.c_str(), O_RDWR | O_NOCTTY);
    if (this->m_fd < 0) {
        this->m_stats.txErrors += 1U;
        return false;
    }

    if (!configureRawFd(this->m_fd, this->m_baudrate)) {
        this->m_stats.txErrors += 1U;
        closeFd(this->m_fd);
        return false;
    }

    this->m_stats.connected = true;
    return true;
}

void SerialByteStreamTransport::disconnect() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->disconnectUnlocked_();
}

void SerialByteStreamTransport::disconnectUnlocked_() {
    closeFd(this->m_fd);
    this->m_stats.connected = false;
}

bool SerialByteStreamTransport::isConnected() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_fd >= 0;
}

ByteStreamStatus SerialByteStreamTransport::exchange(const std::string& request, std::string& response) {
    return this->exchangeDelimited(request, response, '\n');
}

ByteStreamStatus SerialByteStreamTransport::exchangeDelimited(const std::string& request,
                                                              std::string& response,
                                                              char delimiter) {
    std::lock_guard<std::mutex> lock(this->m_mutex);

    if (this->m_fd < 0 && !this->connectUnlocked_()) {
        return ByteStreamStatus::IO_ERROR;
    }

    const ByteStreamStatus writeStatus = writeAll(this->m_fd, request, this->m_timeoutMs);
    if (writeStatus != ByteStreamStatus::OK) {
        this->m_stats.txErrors += 1U;
        this->disconnectUnlocked_();
        return writeStatus;
    }
    this->m_stats.txBytes += static_cast<std::uint32_t>(request.size());

    const ByteStreamStatus status = readDelimited(this->m_fd, this->m_timeoutMs, response, delimiter);
    if (status == ByteStreamStatus::OK) {
        this->m_stats.rxBytes += static_cast<std::uint32_t>(response.size() + 1U);
        return status;
    }

    this->m_stats.rxErrors += 1U;
    if (status == ByteStreamStatus::IO_ERROR) {
        this->disconnectUnlocked_();
    }
    return status;
}

ByteStreamStatus SerialByteStreamTransport::send(const std::uint8_t* data, std::size_t size) {
    std::lock_guard<std::mutex> lock(this->m_mutex);

    if (data == nullptr || size == 0U) {
        return ByteStreamStatus::IO_ERROR;
    }

    if (this->m_fd < 0 && !this->connectUnlocked_()) {
        return ByteStreamStatus::IO_ERROR;
    }

    const ByteStreamStatus writeStatus = writeAll(this->m_fd, data, size, this->m_timeoutMs);
    if (writeStatus != ByteStreamStatus::OK) {
        this->m_stats.txErrors += 1U;
        this->disconnectUnlocked_();
        return writeStatus;
    }
    this->m_stats.txBytes += static_cast<std::uint32_t>(size);
    return ByteStreamStatus::OK;
}

ByteStreamStats SerialByteStreamTransport::getStats() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_stats;
}

}  // namespace COMM
}  // namespace OBC
