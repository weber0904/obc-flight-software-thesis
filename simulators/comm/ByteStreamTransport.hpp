#ifndef OBC_COMM_ByteStreamTransport_HPP
#define OBC_COMM_ByteStreamTransport_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>

namespace OBC {
namespace COMM {

enum class ByteStreamStatus {
    OK,
    TIMEOUT,
    IO_ERROR
};

struct ByteStreamStats {
    std::uint32_t txBytes;
    std::uint32_t rxBytes;
    std::uint32_t txErrors;
    std::uint32_t rxErrors;
    bool connected;
};

class IByteStreamTransport {
  public:
    virtual ~IByteStreamTransport() = default;

    virtual bool connect() = 0;

    virtual void disconnect() = 0;

    virtual bool isConnected() const = 0;

    virtual ByteStreamStatus exchange(const std::string& request, std::string& response) = 0;

    virtual ByteStreamStatus exchangeDelimited(const std::string& request,
                                               std::string& response,
                                               char delimiter) = 0;

    virtual ByteStreamStatus send(const std::uint8_t* data, std::size_t size) = 0;

    virtual ByteStreamStats getStats() const = 0;
};

class TcpByteStreamTransport final : public IByteStreamTransport {
  public:
    TcpByteStreamTransport(const std::string& host, std::uint16_t port, std::uint32_t timeoutMs = 200U);

    ~TcpByteStreamTransport() override;

    bool connect() override;

    void disconnect() override;

    bool isConnected() const override;

    ByteStreamStatus exchange(const std::string& request, std::string& response) override;

    ByteStreamStatus exchangeDelimited(const std::string& request,
                                      std::string& response,
                                      char delimiter) override;

    ByteStreamStatus send(const std::uint8_t* data, std::size_t size) override;

    ByteStreamStats getStats() const override;

  private:
    bool connectUnlocked_();

    void disconnectUnlocked_();

    std::string m_host;
    std::uint16_t m_port;
    std::uint32_t m_timeoutMs;
    int m_fd;
    ByteStreamStats m_stats;
    mutable std::mutex m_mutex;
};

class SerialByteStreamTransport final : public IByteStreamTransport {
  public:
    explicit SerialByteStreamTransport(const std::string& devicePath,
                                       std::uint32_t timeoutMs = 200U,
                                       std::uint32_t baudrate = 115200U);

    ~SerialByteStreamTransport() override;

    bool connect() override;

    void disconnect() override;

    bool isConnected() const override;

    ByteStreamStatus exchange(const std::string& request, std::string& response) override;

    ByteStreamStatus exchangeDelimited(const std::string& request,
                                      std::string& response,
                                      char delimiter) override;

    ByteStreamStatus send(const std::uint8_t* data, std::size_t size) override;

    ByteStreamStats getStats() const override;

  private:
    bool connectUnlocked_();

    void disconnectUnlocked_();

    std::string m_devicePath;
    std::uint32_t m_timeoutMs;
    std::uint32_t m_baudrate;
    int m_fd;
    ByteStreamStats m_stats;
    mutable std::mutex m_mutex;
};

using PtyByteStreamTransport = SerialByteStreamTransport;

}  // namespace COMM
}  // namespace OBC

#endif
