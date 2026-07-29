#include "simulators/comm/StreamIo.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

namespace OBC {
namespace COMM {

namespace {

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

bool configureRawSerialFd(int fd, std::uint32_t baudrate) {
    struct termios tty = {};
    if (::tcgetattr(fd, &tty) != 0) {
        return false;
    }

    speed_t speed = B115200;
    if (!lookupBaudrate(baudrate, speed)) {
        return false;
    }

    ::cfmakeraw(&tty);
    // Match the repo's stable UART paths: ignore modem control and keep RX enabled
    // so transient carrier-line behavior does not masquerade as link loss.
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8 | CLOCAL | CREAD;
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    if (::cfsetispeed(&tty, speed) != 0 || ::cfsetospeed(&tty, speed) != 0) {
        return false;
    }

    return ::tcsetattr(fd, TCSANOW, &tty) == 0;
}

bool configureTcpSocketFd(int fd) {
#ifdef SO_NOSIGPIPE
    int opt = 1;
    return ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt)) == 0;
#else
    (void)fd;
    return true;
#endif
}

bool isSocketFd(int fd) {
    int type = 0;
    socklen_t length = sizeof(type);
    return ::getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &length) == 0;
}

ssize_t writeFdNoSigpipe(int fd, const std::uint8_t* data, std::size_t size) {
    if (!isSocketFd(fd)) {
        return ::write(fd, data, size);
    }
#ifdef MSG_NOSIGNAL
    return ::send(fd, data, size, MSG_NOSIGNAL);
#else
    return ::send(fd, data, size, 0);
#endif
}

}  // namespace

bool resolveIpv4Host(const std::string& input, std::string& output) {
    if (input.empty()) {
        return false;
    }

    in_addr directAddress = {};
    if (::inet_pton(AF_INET, input.c_str(), &directAddress) == 1) {
        output = input;
        return true;
    }

    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (::getaddrinfo(input.c_str(), nullptr, &hints, &result) != 0 || result == nullptr) {
        if (result != nullptr) {
            ::freeaddrinfo(result);
        }
        return false;
    }

    const auto* ipv4Address = reinterpret_cast<const sockaddr_in*>(result->ai_addr);
    char buffer[INET_ADDRSTRLEN] = {};
    const char* converted = ::inet_ntop(AF_INET, &ipv4Address->sin_addr, buffer, sizeof(buffer));
    if (converted != nullptr) {
        output = converted;
    }

    ::freeaddrinfo(result);
    return converted != nullptr;
}

bool openTcpClient(const std::string& host, std::uint16_t port, int& fdOut) {
    fdOut = -1;

    std::string resolvedHost;
    if (!resolveIpv4Host(host, resolvedHost)) {
        return false;
    }

    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }
    if (!configureTcpSocketFd(fd)) {
        ::close(fd);
        return false;
    }

    struct sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (::inet_pton(AF_INET, resolvedHost.c_str(), &address.sin_addr) != 1) {
        ::close(fd);
        return false;
    }

    if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        ::close(fd);
        return false;
    }

    fdOut = fd;
    return true;
}

bool openTcpServer(const std::string& bindHost, std::uint16_t port, int& fdOut) {
    fdOut = -1;

    std::string resolvedHost;
    if (!resolveIpv4Host(bindHost, resolvedHost)) {
        return false;
    }

    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return false;
    }
    if (!configureTcpSocketFd(fd)) {
        ::close(fd);
        return false;
    }

    int opt = 1;
    (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address = {};
#ifdef __APPLE__
    address.sin_len = sizeof(address);
#endif
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (::inet_pton(AF_INET, resolvedHost.c_str(), &address.sin_addr) != 1) {
        ::close(fd);
        return false;
    }

    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
        ::listen(fd, 1) != 0) {
        ::close(fd);
        return false;
    }

    fdOut = fd;
    return true;
}

bool acceptTcpClient(int listenFd, std::uint32_t timeoutMs, int& fdOut) {
    fdOut = -1;
    if (listenFd < 0) {
        return false;
    }

    struct pollfd pollFd = {};
    pollFd.fd = listenFd;
    pollFd.events = POLLIN;
    const int pollStatus = ::poll(&pollFd, 1, static_cast<int>(timeoutMs));
    if (pollStatus <= 0 || (pollFd.revents & POLLIN) == 0) {
        return false;
    }

    int fd = -1;
    do {
        fd = ::accept(listenFd, nullptr, nullptr);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
        return false;
    }
    if (!configureTcpSocketFd(fd)) {
        ::close(fd);
        return false;
    }

    fdOut = fd;
    return true;
}

bool openRawSerial(const std::string& devicePath, std::uint32_t baudrate, int& fdOut) {
    fdOut = -1;
    const int fd = ::open(devicePath.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
        std::cerr << "openRawSerial: open failed device=" << devicePath << " baudrate=" << baudrate
                  << " errno=" << errno << " message=" << std::strerror(errno) << "\n";
        return false;
    }

    if (!configureRawSerialFd(fd, baudrate)) {
        std::cerr << "openRawSerial: configure failed device=" << devicePath << " baudrate=" << baudrate
                  << " errno=" << errno << " message=" << std::strerror(errno) << "\n";
        ::close(fd);
        return false;
    }

    fdOut = fd;
    return true;
}

bool setFdNonBlocking(int fd) {
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void closeFd(int& fd) {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

bool writeAll(int fd, const std::uint8_t* data, std::size_t size) {
    std::size_t offset = 0U;
    while (offset < size) {
        const ssize_t written = writeFdNoSigpipe(fd, data + offset, size - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (written == 0) {
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

StreamReadStatus readSome(int fd,
                          std::uint32_t timeoutMs,
                          std::uint8_t* buffer,
                          std::size_t capacity,
                          std::size_t& bytesRead) {
    bytesRead = 0U;
    if (fd < 0 || buffer == nullptr || capacity == 0U) {
        return StreamReadStatus::IO_ERROR;
    }

    struct pollfd pfd = {};
    pfd.fd = fd;
    pfd.events = POLLIN;

    const int pollResult = ::poll(&pfd, 1, static_cast<int>(timeoutMs));
    if (pollResult == 0) {
        return StreamReadStatus::TIMEOUT;
    }
    if (pollResult < 0) {
        if (errno == EINTR) {
            return StreamReadStatus::TIMEOUT;
        }
        return StreamReadStatus::IO_ERROR;
    }
    if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        return StreamReadStatus::CLOSED;
    }

    const ssize_t readResult = ::read(fd, buffer, capacity);
    if (readResult < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            return StreamReadStatus::TIMEOUT;
        }
        return StreamReadStatus::IO_ERROR;
    }
    if (readResult == 0) {
        return StreamReadStatus::CLOSED;
    }

    bytesRead = static_cast<std::size_t>(readResult);
    return StreamReadStatus::OK;
}

}  // namespace COMM
}  // namespace OBC
