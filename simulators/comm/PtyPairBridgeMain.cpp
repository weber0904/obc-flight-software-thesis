#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>

namespace {

std::atomic<bool> g_stopRequested(false);

void handleSignal(int) {
    g_stopRequested.store(true);
}

bool configureRawPtyFd(int fd) {
    if (fd < 0) {
        return false;
    }
    struct termios tty = {};
    if (::tcgetattr(fd, &tty) != 0) {
        return false;
    }
    ::cfmakeraw(&tty);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    if (::tcsetattr(fd, TCSANOW, &tty) != 0) {
        return false;
    }
    return ::tcflush(fd, TCIOFLUSH) == 0;
}

std::size_t readEnvUsize(const char* name, std::size_t fallback) {
    const char* const raw = std::getenv(name);
    if (raw == nullptr || *raw == '\0') {
        return fallback;
    }
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(raw, &end, 10);
    if (end == raw || (end != nullptr && *end != '\0') || parsed == 0UL) {
        return fallback;
    }
    return static_cast<std::size_t>(parsed);
}

class PtyPair {
  public:
    PtyPair() : m_masterFd(-1), m_slaveHoldFd(-1), m_slavePath() {
        this->m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (this->m_masterFd < 0) {
            return;
        }
        if (::grantpt(this->m_masterFd) != 0 || ::unlockpt(this->m_masterFd) != 0) {
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
            return;
        }
        char* const slave = ::ptsname(this->m_masterFd);
        this->m_slavePath = slave == nullptr ? "" : slave;
        if (this->m_slavePath.empty()) {
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
            this->m_slavePath.clear();
            return;
        }

        this->m_slaveHoldFd = ::open(this->m_slavePath.c_str(), O_RDWR | O_NOCTTY);
        if (this->m_slaveHoldFd < 0) {
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
            this->m_slavePath.clear();
            return;
        }
        const bool configuredSlave = configureRawPtyFd(this->m_slaveHoldFd);
        const bool configuredMaster = configuredSlave && configureRawPtyFd(this->m_masterFd);
        if (!configuredMaster) {
            ::close(this->m_slaveHoldFd);
            this->m_slaveHoldFd = -1;
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
            this->m_slavePath.clear();
        }
    }

    ~PtyPair() {
        if (this->m_slaveHoldFd >= 0) {
            ::close(this->m_slaveHoldFd);
            this->m_slaveHoldFd = -1;
        }
        if (this->m_masterFd >= 0) {
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
        }
    }

    bool valid() const {
        return this->m_masterFd >= 0 && !this->m_slavePath.empty();
    }

    int masterFd() const {
        return this->m_masterFd;
    }

    const std::string& slavePath() const {
        return this->m_slavePath;
    }

  private:
    int m_masterFd;
    int m_slaveHoldFd;
    std::string m_slavePath;
};

bool writeAll(int fd, const std::uint8_t* data, std::size_t size) {
    std::size_t offset = 0U;
    while (offset < size) {
        const ssize_t written = ::write(fd, data + offset, size - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

void bridgeTraffic(int sourceFd, int destinationFd, std::size_t chunkSize, std::size_t interChunkUs) {
    std::uint8_t buffer[512] = {};
    const ssize_t bytesRead = ::read(sourceFd, buffer, sizeof(buffer));
    if (bytesRead <= 0) {
        return;
    }
    const std::size_t total = static_cast<std::size_t>(bytesRead);
    if (chunkSize == 0U || chunkSize >= total) {
        static_cast<void>(writeAll(destinationFd, buffer, total));
        return;
    }
    std::size_t offset = 0U;
    while (offset < total) {
        const std::size_t step = std::min(chunkSize, total - offset);
        if (!writeAll(destinationFd, buffer + offset, step)) {
            return;
        }
        offset += step;
        if (offset < total && interChunkUs > 0U) {
            ::usleep(static_cast<useconds_t>(interChunkUs));
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--instance-label") {
            if (i + 1 >= argc) {
                std::cerr << "--instance-label requires a value\n";
                return 1;
            }
            ++i;
            continue;
        }
        if (arg == "--help") {
            std::cout << "Usage: pty_pair_bridge [--instance-label LABEL]\n";
            return 0;
        }
        std::cerr << "Unknown argument: " << arg << "\n";
        return 1;
    }

    const std::size_t chunkSize = readEnvUsize("PTY_BRIDGE_CHUNK_SIZE", 0U);
    const std::size_t interChunkUs = readEnvUsize("PTY_BRIDGE_INTER_CHUNK_US", 0U);

    PtyPair first;
    PtyPair second;
    if (!first.valid() || !second.valid()) {
        std::cerr << "Failed to create PTY bridge pairs\n";
        return 1;
    }

    std::cout << "PTY_A=" << first.slavePath() << "\n";
    std::cout << "PTY_B=" << second.slavePath() << "\n";
    std::cout.flush();

    while (!g_stopRequested.load()) {
        pollfd fds[2] = {};
        fds[0].fd = first.masterFd();
        fds[0].events = POLLIN;
        fds[1].fd = second.masterFd();
        fds[1].events = POLLIN;

        const int pollStatus = ::poll(fds, 2, 100);
        if (pollStatus < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (pollStatus == 0) {
            continue;
        }

        if ((fds[0].revents & POLLIN) != 0) {
            bridgeTraffic(first.masterFd(), second.masterFd(), chunkSize, interChunkUs);
        }
        if ((fds[1].revents & POLLIN) != 0) {
            bridgeTraffic(second.masterFd(), first.masterFd(), chunkSize, interChunkUs);
        }
    }

    return 0;
}
