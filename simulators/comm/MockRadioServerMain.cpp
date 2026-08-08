#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <termios.h>
#include <unistd.h>

#include "simulators/comm/TransparentLinkFraming.hpp"

namespace {

struct MockRadioState {
    bool enabled = false;
    std::uint8_t powerDbm = 10U;
    std::uint32_t freqHz = 437000000U;
    float tempC = 32.5F;
    std::int16_t rssiDbm = -68;
};

std::string bytesToHex(const std::string& payload) {
    static const char* digits = "0123456789ABCDEF";
    std::string hex;
    hex.reserve(payload.size() * 2U);
    for (unsigned char value : payload) {
        hex.push_back(digits[(value >> 4U) & 0x0FU]);
        hex.push_back(digits[value & 0x0FU]);
    }
    return hex;
}

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

enum class SessionEndReason {
    PEER_CLOSED,
    WRITE_FAILED,
    MAX_FRAMES_REACHED,
};

const char* sessionEndReasonName(SessionEndReason reason) {
    switch (reason) {
        case SessionEndReason::PEER_CLOSED:
            return "peer-closed";
        case SessionEndReason::WRITE_FAILED:
            return "write-failed";
        case SessionEndReason::MAX_FRAMES_REACHED:
            return "max-frames";
        default:
            return "unknown";
    }
}

bool readDelimited(int fd, std::string& payload, char delimiter) {
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

std::string handleTransparentExchange(const std::string& line) {
    return line + "\n";
}

std::string handleTransparentFramedExchange(const std::string& encodedFrame, std::size_t frameIndex) {
    std::string payload;
    const OBC::COMM::TransparentFrameStatus status =
        OBC::COMM::decodeTransparentLinkFrame(encodedFrame, payload, nullptr);
    if (status != OBC::COMM::TransparentFrameStatus::OK) {
        std::cerr << "transparent-framed-echo decode failed: "
                  << OBC::COMM::transparentFrameStatusName(status) << "\n";
        return std::string();
    }
    std::cerr << "transparent-framed-echo frame=" << frameIndex
              << " rx-bytes=" << payload.size()
              << " hex=" << bytesToHex(payload) << "\n";
    return OBC::COMM::encodeTransparentLinkFrame(payload);
}

int runSession(int fd,
               bool transparentMode,
               bool framedTransparentMode,
               std::size_t maxFramedExchanges) {
    MockRadioState state;
    std::string request;
    std::size_t framedCount = 0U;
    SessionEndReason endReason = SessionEndReason::PEER_CLOSED;
    int exitCode = 0;
    if (framedTransparentMode) {
        std::cerr << "transparent-framed-echo session-start"
                  << " max-frames=" << maxFramedExchanges << "\n";
    }
    while ((framedTransparentMode
                ? readUntilByte(fd, request, OBC::COMM::TRANSPARENT_FRAME_DELIMITER)
                : readDelimited(fd, request, '\n'))) {
        const std::string response = framedTransparentMode
                                         ? handleTransparentFramedExchange(request, framedCount + 1U)
                                         : (transparentMode ? handleTransparentExchange(request)
                                                            : handleCommand(request, state));
        if (framedTransparentMode && response.empty()) {
            continue;
        }
        if (!writeAll(fd, response)) {
            endReason = SessionEndReason::WRITE_FAILED;
            exitCode = 1;
            break;
        }
        if (framedTransparentMode) {
            ++framedCount;
            if (maxFramedExchanges > 0U && framedCount >= maxFramedExchanges) {
                endReason = SessionEndReason::MAX_FRAMES_REACHED;
                break;
            }
        }
    }
    if (framedTransparentMode) {
        std::cerr << "transparent-framed-echo session-end"
                  << " frames=" << framedCount
                  << " reason=" << sessionEndReasonName(endReason) << "\n";
    }
    return exitCode;
}

int runTcp(std::uint16_t port,
           bool transparentMode,
           bool framedTransparentMode,
           std::size_t maxFramedExchanges) {
    const int listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0) {
        return 1;
    }

    int opt = 1;
    (void)::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (::bind(listenFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0 ||
        ::listen(listenFd, 1) != 0) {
        (void)::close(listenFd);
        return 1;
    }

    const char* modeName = framedTransparentMode ? "transparent-framed-echo"
                                                 : (transparentMode ? "transparent-echo" : "mock-text");
    std::cout << "radio_mock_server listening on 127.0.0.1:" << port
              << " mode=" << modeName << "\n";
    const int clientFd = ::accept(listenFd, nullptr, nullptr);
    if (clientFd < 0) {
        (void)::close(listenFd);
        return 1;
    }

    const int status = runSession(clientFd, transparentMode, framedTransparentMode, maxFramedExchanges);
    (void)::close(clientFd);
    (void)::close(listenFd);
    return status;
}

int runPty(const std::string& symlinkPath,
           bool transparentMode,
           bool framedTransparentMode,
           std::uint32_t baudrate,
           std::size_t maxFramedExchanges) {
    const int masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (masterFd < 0 || ::grantpt(masterFd) != 0 || ::unlockpt(masterFd) != 0) {
        return 1;
    }

    char* slave = ::ptsname(masterFd);
    if (slave == nullptr) {
        (void)::close(masterFd);
        return 1;
    }

    const int slaveFd = ::open(slave, O_RDWR | O_NOCTTY);
    if (slaveFd < 0 || !configureRawFd(slaveFd, baudrate)) {
        if (slaveFd >= 0) {
            (void)::close(slaveFd);
        }
        (void)::close(masterFd);
        return 1;
    }

    if (!symlinkPath.empty()) {
        (void)::unlink(symlinkPath.c_str());
        if (::symlink(slave, symlinkPath.c_str()) != 0) {
            std::perror("symlink");
        } else {
            std::cout << "radio_mock_server symlinked " << symlinkPath << " -> " << slave << "\n";
        }
    }

    const char* modeName = framedTransparentMode ? "transparent-framed-echo"
                                                 : (transparentMode ? "transparent-echo" : "mock-text");
    std::cout << "radio_mock_server PTY slave " << slave
              << " mode=" << modeName
              << " baudrate=" << baudrate << "\n";
    const int status = runSession(masterFd, transparentMode, framedTransparentMode, maxFramedExchanges);
    (void)::close(slaveFd);
    (void)::close(masterFd);
    return status;
}

int runSerial(const std::string& devicePath,
              bool transparentMode,
              bool framedTransparentMode,
              std::uint32_t baudrate,
              std::size_t maxFramedExchanges) {
    const int fd = ::open(devicePath.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0 || !configureRawFd(fd, baudrate)) {
        if (fd >= 0) {
            (void)::close(fd);
        }
        return 1;
    }

    const char* modeName = framedTransparentMode ? "transparent-framed-echo"
                                                 : (transparentMode ? "transparent-echo" : "mock-text");
    std::cout << "radio_mock_server serial device " << devicePath
              << " mode=" << modeName
              << " baudrate=" << baudrate << "\n";
    const int status = runSession(fd, transparentMode, framedTransparentMode, maxFramedExchanges);
    (void)::close(fd);
    return status;
}

void printUsage(const char* app) {
    std::cout << "Usage: " << app
              << " [--port 7000] [--pty-link /tmp/obc-radio] [--serial-device /path/to/tty]"
              << " [--mode mock-text|transparent-echo|transparent-framed-echo]"
              << " [--baudrate 115200] [--max-framed-exchanges 0]\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::uint16_t port = 7000U;
    std::uint32_t baudrate = 115200U;
    std::size_t maxFramedExchanges = 0U;
    std::string ptyLink;
    std::string serialDevice;
    std::string mode = "mock-text";

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if ((arg == "--help") || (arg == "-h")) {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "--port" && i + 1 < argc) {
            port = static_cast<std::uint16_t>(std::strtoul(argv[++i], nullptr, 10));
            continue;
        }
        if (arg == "--pty-link" && i + 1 < argc) {
            ptyLink = argv[++i];
            continue;
        }
        if (arg == "--serial-device" && i + 1 < argc) {
            serialDevice = argv[++i];
            continue;
        }
        if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
            continue;
        }
        if (arg == "--baudrate" && i + 1 < argc) {
            baudrate = static_cast<std::uint32_t>(std::strtoul(argv[++i], nullptr, 10));
            continue;
        }
        if (arg == "--max-framed-exchanges" && i + 1 < argc) {
            maxFramedExchanges = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
            continue;
        }
        std::cerr << "Unknown argument: " << arg << "\n";
        printUsage(argv[0]);
        return 1;
    }

    if (mode != "mock-text" && mode != "transparent-echo" && mode != "transparent-framed-echo") {
        std::cerr << "Unsupported --mode: " << mode << "\n";
        return 1;
    }
    const bool transparentMode = mode == "transparent-echo";
    const bool framedTransparentMode = mode == "transparent-framed-echo";

    if (!ptyLink.empty() && !serialDevice.empty()) {
        std::cerr << "--pty-link and --serial-device are mutually exclusive\n";
        return 1;
    }
    if (!ptyLink.empty()) {
        return runPty(ptyLink, transparentMode, framedTransparentMode, baudrate, maxFramedExchanges);
    }
    if (!serialDevice.empty()) {
        return runSerial(serialDevice, transparentMode, framedTransparentMode, baudrate, maxFramedExchanges);
    }
    return runTcp(port, transparentMode, framedTransparentMode, maxFramedExchanges);
}
