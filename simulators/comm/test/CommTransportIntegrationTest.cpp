#include "simulators/comm/RadioTransport.hpp"
#include "simulators/comm/TransparentLinkFraming.hpp"

#include <arpa/inet.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace {

struct MockRadioState {
    bool enabled = false;
    std::uint8_t powerDbm = 10U;
    std::uint32_t freqHz = 437000000U;
    float tempC = 32.5F;
    std::int16_t rssiDbm = -68;
};

bool writeAll(int fd, const std::string& payload) {
    std::size_t offset = 0U;
    while (offset < payload.size()) {
        const ssize_t written = ::write(fd, payload.data() + offset, payload.size() - offset);
        if (written <= 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

bool readLine(int fd, std::string& line) {
    line.clear();
    char ch = 0;
    while (true) {
        const ssize_t bytesRead = ::read(fd, &ch, 1);
        if (bytesRead <= 0) {
            if (bytesRead < 0 && errno == EINTR) {
                continue;
            }
            return false;
        }
        if (ch == '\n') {
            return true;
        }
        line.push_back(ch);
    }
}

bool readUntilByte(int fd, std::string& payload, char delimiter) {
    payload.clear();
    char ch = 0;
    while (true) {
        const ssize_t bytesRead = ::read(fd, &ch, 1);
        if (bytesRead <= 0) {
            if (bytesRead < 0 && errno == EINTR) {
                continue;
            }
            return false;
        }
        if (ch == delimiter) {
            return true;
        }
        payload.push_back(ch);
    }
}

bool writeEchoLine(int fd, const std::string& line) {
    return writeAll(fd, line + "\n");
}

std::string renderStatus(const char* prefix, const MockRadioState& state) {
    std::ostringstream output;
    output << prefix << " enabled=" << (state.enabled ? 1 : 0) << " power=" << static_cast<unsigned int>(state.powerDbm)
           << " freq=" << state.freqHz << " temp=" << state.tempC << " rssi=" << state.rssiDbm << "\n";
    return output.str();
}

std::string handleCommand(const std::string& line, MockRadioState& state) {
    std::istringstream input(line);
    std::string command;
    input >> command;

    if (command == "STATUS") {
        return renderStatus("STATUS", state);
    }

    if (command == "ENABLE") {
        int enabled = 0;
        input >> enabled;
        state.enabled = enabled != 0;
        return renderStatus("OK", state);
    }

    if (command == "POWER") {
        unsigned int powerDbm = 0U;
        input >> powerDbm;
        state.powerDbm = static_cast<std::uint8_t>(powerDbm);
        return renderStatus("OK", state);
    }

    if (command == "FREQ") {
        std::uint32_t freqHz = 0U;
        input >> freqHz;
        state.freqHz = freqHz;
        return renderStatus("OK", state);
    }

    return "ERR\n";
}

void serveConnection(int fd) {
    MockRadioState state;
    std::string request;
    while (readLine(fd, request)) {
        if (!writeAll(fd, handleCommand(request, state))) {
            break;
        }
    }
    (void)::close(fd);
}

void serveTransparentPeer(int fd) {
    std::string request;
    while (readLine(fd, request)) {
        if (!writeEchoLine(fd, request)) {
            break;
        }
    }
    (void)::close(fd);
}

void serveTransparentFramedPeer(int fd, std::size_t maxFrames = 0U) {
    std::string encodedRequest;
    std::size_t frameCount = 0U;
    while (readUntilByte(fd, encodedRequest, OBC::COMM::TRANSPARENT_FRAME_DELIMITER)) {
        std::string payload;
        const OBC::COMM::TransparentFrameStatus status =
            OBC::COMM::decodeTransparentLinkFrame(encodedRequest, payload, nullptr);
        assert(status == OBC::COMM::TransparentFrameStatus::OK);
        if (!writeAll(fd, OBC::COMM::encodeTransparentLinkFrame(payload))) {
            break;
        }
        ++frameCount;
        if (maxFrames > 0U && frameCount >= maxFrames) {
            break;
        }
    }
    (void)::close(fd);
}

class TcpServer {
  public:
    TcpServer() : m_listenFd(-1), m_port(0U) {
        this->m_listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
        assert(this->m_listenFd >= 0);

        int opt = 1;
        (void)::setsockopt(this->m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(0U);
        assert(::bind(this->m_listenFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0);
        assert(::listen(this->m_listenFd, 1) == 0);

        socklen_t addrLen = sizeof(addr);
        assert(::getsockname(this->m_listenFd, reinterpret_cast<struct sockaddr*>(&addr), &addrLen) == 0);
        this->m_port = ntohs(addr.sin_port);

        this->m_thread = std::thread([this]() {
            const int clientFd = ::accept(this->m_listenFd, nullptr, nullptr);
            assert(clientFd >= 0);
            serveConnection(clientFd);
        });
    }

    ~TcpServer() {
        if (this->m_listenFd >= 0) {
            (void)::close(this->m_listenFd);
            this->m_listenFd = -1;
        }
        if (this->m_thread.joinable()) {
            this->m_thread.join();
        }
    }

    std::uint16_t port() const {
        return this->m_port;
    }

  private:
    int m_listenFd;
    std::uint16_t m_port;
    std::thread m_thread;
};

class PtyPeer {
  public:
    PtyPeer() : m_masterFd(-1) {
        this->m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
        assert(this->m_masterFd >= 0);
        assert(::grantpt(this->m_masterFd) == 0);
        assert(::unlockpt(this->m_masterFd) == 0);
        char* slave = ::ptsname(this->m_masterFd);
        assert(slave != nullptr);
        this->m_slavePath = slave;

        this->m_thread = std::thread([this]() { serveConnection(this->m_masterFd); });
    }

    ~PtyPeer() {
        if (this->m_thread.joinable()) {
            this->m_thread.join();
        }
    }

    const std::string& slavePath() const {
        return this->m_slavePath;
    }

  private:
    int m_masterFd;
    std::string m_slavePath;
    std::thread m_thread;
};

class TransparentPtyPeer {
  public:
    TransparentPtyPeer() : m_masterFd(-1) {
        this->m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
        assert(this->m_masterFd >= 0);
        assert(::grantpt(this->m_masterFd) == 0);
        assert(::unlockpt(this->m_masterFd) == 0);
        char* slave = ::ptsname(this->m_masterFd);
        assert(slave != nullptr);
        this->m_slavePath = slave;

        this->m_thread = std::thread([this]() { serveTransparentPeer(this->m_masterFd); });
    }

    ~TransparentPtyPeer() {
        if (this->m_thread.joinable()) {
            this->m_thread.join();
        }
    }

    const std::string& slavePath() const {
        return this->m_slavePath;
    }

  private:
    int m_masterFd;
    std::string m_slavePath;
    std::thread m_thread;
};

class TransparentFramedPtyPeer {
  public:
    explicit TransparentFramedPtyPeer(std::size_t maxFrames = 0U) : m_masterFd(-1) {
        this->m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
        assert(this->m_masterFd >= 0);
        assert(::grantpt(this->m_masterFd) == 0);
        assert(::unlockpt(this->m_masterFd) == 0);
        char* slave = ::ptsname(this->m_masterFd);
        assert(slave != nullptr);
        this->m_slavePath = slave;

        this->m_thread = std::thread([this, maxFrames]() { serveTransparentFramedPeer(this->m_masterFd, maxFrames); });
    }

    ~TransparentFramedPtyPeer() {
        if (this->m_thread.joinable()) {
            this->m_thread.join();
        }
    }

    const std::string& slavePath() const {
        return this->m_slavePath;
    }

  private:
    int m_masterFd;
    std::string m_slavePath;
    std::thread m_thread;
};

void verifyTransport(OBC::COMM::IRadioTransport& transport) {
    OBC::COMM::RadioStatus status = {};

    assert(transport.getStatus(status) == OBC::COMM::RadioTransportStatus::OK);
    assert(status.enabled == false);
    assert(status.powerDbm == 10U);

    assert(transport.setEnabled(true, status) == OBC::COMM::RadioTransportStatus::OK);
    assert(status.enabled == true);

    assert(transport.setPower(22U, status) == OBC::COMM::RadioTransportStatus::OK);
    assert(status.powerDbm == 22U);

    assert(transport.setFrequency(435100000U, status) == OBC::COMM::RadioTransportStatus::OK);
    assert(status.freqHz == 435100000U);
    assert(status.rssiDbm == -68);
}

void verifySharedLink(const std::shared_ptr<OBC::COMM::IByteStreamTransport>& link) {
    OBC::COMM::HostedRadioTransport radio(link, OBC::COMM::makeRadioProtocolAdapter("mock-text"));
    OBC::COMM::RadioStatus status = {};
    std::string response;

    assert(radio.setEnabled(true, status) == OBC::COMM::RadioTransportStatus::OK);
    assert(status.enabled == true);

    assert(link->exchange("STATUS\n", response) == OBC::COMM::ByteStreamStatus::OK);
    assert(response.find("enabled=1") != std::string::npos);

    assert(radio.setPower(18U, status) == OBC::COMM::RadioTransportStatus::OK);
    assert(status.powerDbm == 18U);

    assert(link->exchange("STATUS\n", response) == OBC::COMM::ByteStreamStatus::OK);
    assert(response.find("power=18") != std::string::npos);
}

void verifyTransparentLink(const std::shared_ptr<OBC::COMM::IByteStreamTransport>& link) {
    std::string response;

    assert(link->exchange("ENDUROSAT_PING\n", response) == OBC::COMM::ByteStreamStatus::OK);
    assert(response == "ENDUROSAT_PING");

    assert(link->exchange("PAYLOAD-ALPHA-123\n", response) == OBC::COMM::ByteStreamStatus::OK);
    assert(response == "PAYLOAD-ALPHA-123");
}

void verifyTransparentPassiveProtocolDoesNotTransmit(const std::shared_ptr<OBC::COMM::IByteStreamTransport>& link) {
    OBC::COMM::HostedRadioTransport radio(link, OBC::COMM::makeRadioProtocolAdapter("transparent-passive"));
    OBC::COMM::RadioStatus status = {};
    const OBC::COMM::ByteStreamStats before = link->getStats();

    assert(radio.getStatus(status) == OBC::COMM::RadioTransportStatus::UNSUPPORTED);
    const OBC::COMM::ByteStreamStats after = link->getStats();

    assert(before.txBytes == after.txBytes);
    assert(before.rxBytes == after.rxBytes);
}

std::string makeTransparentBinaryPayload() {
    std::string payload;
    payload.reserve(7U);
    payload.push_back(static_cast<char>(0x00));
    payload.push_back(OBC::COMM::TRANSPARENT_FRAME_DELIMITER);
    payload.push_back(OBC::COMM::TRANSPARENT_FRAME_ESCAPE);
    payload += "ABC";
    payload.push_back(static_cast<char>(0xFF));
    return payload;
}

std::string makeTransparentBinaryPayloadB() {
    std::string payload;
    payload.reserve(9U);
    payload.push_back(static_cast<char>(0x10));
    payload.push_back(static_cast<char>(0x20));
    payload.push_back(static_cast<char>(0x00));
    payload += "END";
    payload.push_back(OBC::COMM::TRANSPARENT_FRAME_ESCAPE);
    payload.push_back(static_cast<char>(0xAA));
    payload.push_back(static_cast<char>(0x55));
    return payload;
}

std::string makeTransparentBinaryPayloadC() {
    std::string payload;
    payload.reserve(12U);
    for (unsigned char value = 1U; value <= 12U; ++value) {
        payload.push_back(static_cast<char>(value));
    }
    return payload;
}

void verifyTransparentFrameCodec() {
    const std::string payload = makeTransparentBinaryPayload();
    const std::string encoded = OBC::COMM::encodeTransparentLinkFrame(payload);
    assert(!encoded.empty());
    assert(encoded.back() == OBC::COMM::TRANSPARENT_FRAME_DELIMITER);

    std::string decoded;
    OBC::COMM::TransparentFrameInfo info = {};
    const OBC::COMM::TransparentFrameStatus status =
        OBC::COMM::decodeTransparentLinkFrame(encoded.substr(0, encoded.size() - 1U), decoded, &info);
    assert(status == OBC::COMM::TransparentFrameStatus::OK);
    assert(info.version == OBC::COMM::TRANSPARENT_FRAME_VERSION);
    assert(decoded == payload);
}

void verifyFramedTransparentLink(const std::shared_ptr<OBC::COMM::IByteStreamTransport>& link) {
    const std::string payloads[] = {
        makeTransparentBinaryPayload(),
        makeTransparentBinaryPayloadB(),
        makeTransparentBinaryPayloadC(),
    };
    for (const std::string& payload : payloads) {
        const std::string request = OBC::COMM::encodeTransparentLinkFrame(payload);
        std::string encodedResponse;
        std::string decodedResponse;

        assert(link->exchangeDelimited(request, encodedResponse, OBC::COMM::TRANSPARENT_FRAME_DELIMITER) ==
               OBC::COMM::ByteStreamStatus::OK);
        assert(OBC::COMM::decodeTransparentLinkFrame(encodedResponse, decodedResponse, nullptr) ==
               OBC::COMM::TransparentFrameStatus::OK);
        assert(decodedResponse == payload);
    }
}

void verifyFramedTransparentRestartBehavior() {
    {
        TransparentFramedPtyPeer peer;
        const std::shared_ptr<OBC::COMM::IByteStreamTransport> link(
            new OBC::COMM::SerialByteStreamTransport(peer.slavePath(), 300U, 230400U));
        const std::string request = OBC::COMM::encodeTransparentLinkFrame(makeTransparentBinaryPayload());
        std::string decodedResponse;
        std::string encodedResponse;

        assert(link->exchangeDelimited(request, encodedResponse, OBC::COMM::TRANSPARENT_FRAME_DELIMITER) ==
               OBC::COMM::ByteStreamStatus::OK);
        assert(OBC::COMM::decodeTransparentLinkFrame(encodedResponse, decodedResponse, nullptr) ==
               OBC::COMM::TransparentFrameStatus::OK);
        assert(decodedResponse == makeTransparentBinaryPayload());
    }

    {
        TransparentFramedPtyPeer peer;
        const std::shared_ptr<OBC::COMM::IByteStreamTransport> link(
            new OBC::COMM::SerialByteStreamTransport(peer.slavePath(), 300U, 230400U));
        verifyFramedTransparentLink(link);
    }
}

}  // namespace

int main() {
    assert(OBC::COMM::isSupportedRadioProtocol("mock-text"));
    assert(!OBC::COMM::isSupportedRadioProtocol("unknown"));
    verifyTransparentFrameCodec();

    {
        TcpServer server;
        std::unique_ptr<OBC::COMM::IRadioTransport> transport =
            OBC::COMM::makeTcpMockRadioTransport("127.0.0.1", server.port(), 300U, "mock-text");
        verifyTransport(*transport);
    }

    {
        TcpServer server;
        const std::shared_ptr<OBC::COMM::IByteStreamTransport> link(
            new OBC::COMM::TcpByteStreamTransport("127.0.0.1", server.port(), 300U));
        verifySharedLink(link);
    }

    {
        PtyPeer peer;
        std::unique_ptr<OBC::COMM::IRadioTransport> transport =
            OBC::COMM::makePtyRadioTransport(peer.slavePath(), 300U, "mock-text");
        verifyTransport(*transport);
    }

    {
        PtyPeer peer;
        const std::shared_ptr<OBC::COMM::IByteStreamTransport> link(
            new OBC::COMM::SerialByteStreamTransport(peer.slavePath(), 300U));
        verifySharedLink(link);
    }

    {
        TransparentPtyPeer peer;
        const std::shared_ptr<OBC::COMM::IByteStreamTransport> link(
            new OBC::COMM::SerialByteStreamTransport(peer.slavePath(), 300U, 230400U));
        assert(OBC::COMM::isSupportedRadioProtocol("transparent-passive"));
        verifyTransparentPassiveProtocolDoesNotTransmit(link);
        verifyTransparentLink(link);
    }

    {
        TransparentFramedPtyPeer peer;
        const std::shared_ptr<OBC::COMM::IByteStreamTransport> link(
            new OBC::COMM::SerialByteStreamTransport(peer.slavePath(), 300U, 230400U));
        verifyFramedTransparentLink(link);
    }

    verifyFramedTransparentRestartBehavior();

    return 0;
}
