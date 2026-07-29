#ifndef OBC_COMM_STREAMIO_HPP
#define OBC_COMM_STREAMIO_HPP

#include <cstddef>
#include <cstdint>
#include <string>

namespace OBC {
namespace COMM {

enum class StreamReadStatus {
    OK,
    TIMEOUT,
    CLOSED,
    IO_ERROR,
};

bool resolveIpv4Host(const std::string& input, std::string& output);

bool openTcpClient(const std::string& host, std::uint16_t port, int& fdOut);

bool openTcpServer(const std::string& bindHost, std::uint16_t port, int& fdOut);

bool acceptTcpClient(int listenFd, std::uint32_t timeoutMs, int& fdOut);

bool openRawSerial(const std::string& devicePath, std::uint32_t baudrate, int& fdOut);

bool setFdNonBlocking(int fd);

void closeFd(int& fd);

bool writeAll(int fd, const std::uint8_t* data, std::size_t size);

inline bool writeAll(int fd, const std::string& payload) {
    return writeAll(fd,
                    reinterpret_cast<const std::uint8_t*>(payload.data()),
                    payload.size());
}

StreamReadStatus readSome(int fd, std::uint32_t timeoutMs, std::uint8_t* buffer, std::size_t capacity, std::size_t& bytesRead);

}  // namespace COMM
}  // namespace OBC

#endif
