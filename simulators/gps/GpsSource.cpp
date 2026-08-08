#include "simulators/gps/GpsSource.hpp"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <limits>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace OBC {
namespace GPS {

namespace {

constexpr std::size_t MAX_SERIAL_BUFFER_LENGTH = 192U;

void normalizeSentence(std::string& sentence) {
    while (!sentence.empty() && (sentence.back() == '\r' || sentence.back() == '\n')) {
        sentence.pop_back();
    }
}

void closeFd(int& fd) {
    if (fd >= 0) {
        (void)::close(fd);
        fd = -1;
    }
}

bool lookupBaudrate(const std::uint32_t baudrate, speed_t& speed) {
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
        default:
            return false;
    }
}

bool configureRawFd(const int fd, const std::uint32_t baudrate) {
    struct termios tty;
    speed_t speed = B9600;
    if (::tcgetattr(fd, &tty) != 0) {
        return false;
    }
    if (!lookupBaudrate(baudrate, speed)) {
        return false;
    }
    ::cfmakeraw(&tty);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0;
    if (::cfsetispeed(&tty, speed) != 0 || ::cfsetospeed(&tty, speed) != 0) {
        return false;
    }
    return ::tcsetattr(fd, TCSANOW, &tty) == 0;
}

}  // namespace

FakeGpsSentenceSource::FakeGpsSentenceSource(std::vector<std::string> sentences)
    : m_sentences(std::move(sentences)), m_index(0U) {}

bool FakeGpsSentenceSource::nextSentence(std::string& sentence) {
    if (this->m_sentences.empty()) {
        sentence.clear();
        return false;
    }
    sentence = this->m_sentences[this->m_index];
    this->m_index = (this->m_index + 1U) % this->m_sentences.size();
    return true;
}

void FakeGpsSentenceSource::reset() {
    this->m_index = 0U;
}

ReplayFileGpsSentenceSource::ReplayFileGpsSentenceSource(std::vector<std::string> sentences)
    : m_sentences(std::move(sentences)), m_index(0U) {}

bool ReplayFileGpsSentenceSource::nextSentence(std::string& sentence) {
    if (this->m_sentences.empty()) {
        sentence.clear();
        return false;
    }
    sentence = this->m_sentences[this->m_index];
    this->m_index = (this->m_index + 1U) % this->m_sentences.size();
    return true;
}

void ReplayFileGpsSentenceSource::reset() {
    this->m_index = 0U;
}

SerialGpsSentenceSource::SerialGpsSentenceSource(const int fd, const std::uint32_t timeoutMs)
    : m_fd(fd), m_timeoutMs(timeoutMs), m_buffer(), m_discardUntilNewline(false) {}

SerialGpsSentenceSource::~SerialGpsSentenceSource() {
    closeFd(this->m_fd);
}

bool SerialGpsSentenceSource::nextSentence(std::string& sentence) {
    sentence.clear();

    const std::uint32_t timeoutMs = this->m_timeoutMs == 0U ? 1U : this->m_timeoutMs;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    struct pollfd pfd;
    pfd.fd = this->m_fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    while (true) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return false;
        }

        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        const int pollTimeoutMs =
            remaining <= 0 ? 0 : static_cast<int>(std::min<long long>(remaining, std::numeric_limits<int>::max()));
        const int pollResult = ::poll(&pfd, 1, pollTimeoutMs);
        if (pollResult == 0) {
            return false;
        }
        if (pollResult < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }

        char ch = 0;
        const ssize_t bytesRead = ::read(this->m_fd, &ch, 1);
        if (bytesRead <= 0) {
            if (bytesRead < 0 && errno == EINTR) {
                continue;
            }
            return false;
        }

        if (this->m_discardUntilNewline) {
            if (ch == '\n') {
                this->m_discardUntilNewline = false;
                this->m_buffer.clear();
            }
            continue;
        }

        if (ch == '\n') {
            sentence = this->m_buffer;
            normalizeSentence(sentence);
            this->m_buffer.clear();
            if (sentence.empty()) {
                continue;
            }
            return true;
        }

        this->m_buffer.push_back(ch);
        if (this->m_buffer.size() > MAX_SERIAL_BUFFER_LENGTH) {
            this->m_buffer.clear();
            this->m_discardUntilNewline = true;
            return false;
        }
    }
}

void SerialGpsSentenceSource::reset() {
    this->m_buffer.clear();
    this->m_discardUntilNewline = false;
}

std::unique_ptr<SerialGpsSentenceSource> SerialGpsSentenceSource::openDevice(const std::string& path,
                                                                             const std::uint32_t baudrate,
                                                                             const std::uint32_t timeoutMs) {
    int fd = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return std::unique_ptr<SerialGpsSentenceSource>();
    }
    if (!configureRawFd(fd, baudrate)) {
        closeFd(fd);
        return std::unique_ptr<SerialGpsSentenceSource>();
    }
    return std::unique_ptr<SerialGpsSentenceSource>(new SerialGpsSentenceSource(fd, timeoutMs));
}

std::unique_ptr<ReplayFileGpsSentenceSource> ReplayFileGpsSentenceSource::loadFromFile(const std::string& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return std::unique_ptr<ReplayFileGpsSentenceSource>();
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        normalizeSentence(line);
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    if (lines.empty()) {
        return std::unique_ptr<ReplayFileGpsSentenceSource>();
    }
    return std::unique_ptr<ReplayFileGpsSentenceSource>(new ReplayFileGpsSentenceSource(std::move(lines)));
}

std::vector<std::string> makeDefaultFakeNmeaSequence() {
    return std::vector<std::string>{
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A",
    };
}

std::unique_ptr<IGpsSentenceSource> makeDefaultFakeGpsSentenceSource() {
    return std::unique_ptr<IGpsSentenceSource>(new FakeGpsSentenceSource(makeDefaultFakeNmeaSequence()));
}

std::unique_ptr<IGpsSentenceSource> makeReplayFileGpsSentenceSource(const std::string& path) {
    return ReplayFileGpsSentenceSource::loadFromFile(path);
}

std::unique_ptr<IGpsSentenceSource> makeSerialGpsSentenceSource(const std::string& path,
                                                                const std::uint32_t baudrate,
                                                                const std::uint32_t timeoutMs) {
    return SerialGpsSentenceSource::openDevice(path, baudrate, timeoutMs);
}

}  // namespace GPS
}  // namespace OBC
