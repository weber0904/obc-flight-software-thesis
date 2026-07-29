#include <cstdint>
#include <cstdio>
#include <chrono>
#include <fcntl.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>

#include "simulators/gps/GpsSource.hpp"
#include "simulators/gps/NmeaParser.hpp"

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

std::string makeSentence(const std::string& payload) {
    std::uint8_t checksum = 0U;
    for (char ch : payload) {
        checksum ^= static_cast<std::uint8_t>(ch);
    }

    char suffix[8] = {};
    std::snprintf(suffix, sizeof(suffix), "*%02X", static_cast<unsigned int>(checksum));
    return "$" + payload + suffix;
}

std::string makeTempReplayPath() {
    char buffer[] = "/tmp/gps-replay-XXXXXX.txt";
    int fd = ::mkstemps(buffer, 4);
    if (fd >= 0) {
        ::close(fd);
    }
    return std::string(buffer);
}

bool writeAll(int fd, const std::string& payload) {
    std::size_t offset = 0U;
    while (offset < payload.size()) {
        const ssize_t written = ::write(fd, payload.data() + offset, payload.size() - offset);
        if (written <= 0) {
            return false;
        }
        offset += static_cast<std::size_t>(written);
    }
    return true;
}

class PtyPair {
  public:
    PtyPair() : m_masterFd(-1) {
        this->m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (!check(this->m_masterFd >= 0, "Expected PTY master fd")) {
            return;
        }
        if (!check(::grantpt(this->m_masterFd) == 0, "Expected grantpt success")) {
            return;
        }
        if (!check(::unlockpt(this->m_masterFd) == 0, "Expected unlockpt success")) {
            return;
        }
        char* slave = ::ptsname(this->m_masterFd);
        if (!check(slave != nullptr, "Expected PTY slave path")) {
            return;
        }
        this->m_slavePath = slave != nullptr ? slave : "";
    }

    ~PtyPair() {
        if (this->m_masterFd >= 0) {
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
        }
    }

    int masterFd() const {
        return this->m_masterFd;
    }

    const std::string& slavePath() const {
        return this->m_slavePath;
    }

  private:
    int m_masterFd;
    std::string m_slavePath;
};

bool testParserHandlesValidGgaAndRmc() {
    bool ok = true;
    OBC::GPS::SentenceUpdate update = {};

    OBC::GPS::ParseStatus status = OBC::GPS::parseNmeaSentence(
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47", update);
    ok = check(status == OBC::GPS::ParseStatus::OK, "Expected valid GGA status") && ok;
    ok = check(update.fixValid, "Expected valid GGA fix") && ok;
    ok = check(update.hasLatitudeLongitude, "Expected GGA latitude/longitude") && ok;
    ok = check(update.hasAltitude, "Expected GGA altitude") && ok;
    ok = check(update.hasSatelliteCount && update.satelliteCount == 8U, "Expected GGA satellites") && ok;
    ok = check(update.hasUtcTime && update.utcSecondsOfDay == 45319U, "Expected GGA UTC time") && ok;

    status = OBC::GPS::parseNmeaSentence(
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A", update);
    ok = check(status == OBC::GPS::ParseStatus::OK, "Expected valid RMC status") && ok;
    ok = check(update.fixValid, "Expected valid RMC fix") && ok;
    ok = check(update.hasSpeed && update.speedMetersPerSecond > 11.0F && update.speedMetersPerSecond < 12.0F,
               "Expected converted RMC speed in m/s") &&
         ok;
    ok = check(update.hasCourse && update.courseDegrees > 84.0F && update.courseDegrees < 85.0F,
               "Expected RMC course") &&
         ok;
    ok = check(update.hasUtcDate && update.utcDateYmd == 19940323U, "Expected RMC date") && ok;
    return ok;
}

bool testParserHandlesNoFixAndChecksumFailure() {
    bool ok = true;
    OBC::GPS::SentenceUpdate update = {};

    OBC::GPS::ParseStatus status =
        OBC::GPS::parseNmeaSentence("$GPGGA,123520,,,,,0,00,99.99,,,,,,*4F", update);
    ok = check(status == OBC::GPS::ParseStatus::NO_FIX, "Expected no-fix GGA status") && ok;
    ok = check(!update.fixValid, "Expected no-fix validity to be false") && ok;
    ok = check(update.hasSatelliteCount && update.satelliteCount == 0U, "Expected no-fix satellite count") && ok;

    status = OBC::GPS::parseNmeaSentence("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00",
                                         update);
    ok = check(status == OBC::GPS::ParseStatus::INVALID_CHECKSUM, "Expected checksum failure") && ok;
    return ok;
}

bool testParserRejectsOversizedOrOverfieldSentence() {
    bool ok = true;
    OBC::GPS::SentenceUpdate update = {};

    std::string oversizedPayload = "GPGGA";
    oversizedPayload.append(120U, '1');
    OBC::GPS::ParseStatus status = OBC::GPS::parseNmeaSentence(makeSentence(oversizedPayload), update);
    ok = check(status == OBC::GPS::ParseStatus::MALFORMED, "Expected oversized sentence to be malformed") && ok;

    std::string tooManyFieldsPayload = "GPGGA";
    for (unsigned int index = 0U; index < 24U; index++) {
        tooManyFieldsPayload += ",1";
    }
    status = OBC::GPS::parseNmeaSentence(makeSentence(tooManyFieldsPayload), update);
    ok = check(status == OBC::GPS::ParseStatus::MALFORMED, "Expected over-field-count sentence to be malformed") && ok;

    return ok;
}

bool testReplaySourceCyclesLoadedSentences() {
    bool ok = true;
    const std::string path = makeTempReplayPath();
    {
        std::ofstream output(path);
        output << "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\n";
        output << "$GPGGA,123520,,,,,0,00,99.99,,,,,,*4F\n";
    }

    std::unique_ptr<OBC::GPS::IGpsSentenceSource> source = OBC::GPS::makeReplayFileGpsSentenceSource(path);
    ok = check(source != nullptr, "Expected replay source to load") && ok;

    std::string first;
    std::string second;
    std::string third;
    ok = check(source->nextSentence(first), "Expected first replay sentence") && ok;
    ok = check(source->nextSentence(second), "Expected second replay sentence") && ok;
    ok = check(source->nextSentence(third), "Expected replay source to cycle") && ok;
    ok = check(first == third, "Expected replay source to cycle back to first sentence") && ok;

    std::remove(path.c_str());
    return ok;
}

bool testSerialSourceTrimsCrLfAndReadsSentence() {
    bool ok = true;
    PtyPair pty;
    std::unique_ptr<OBC::GPS::IGpsSentenceSource> source =
        OBC::GPS::makeSerialGpsSentenceSource(pty.slavePath(), 9600U, 50U);
    ok = check(source != nullptr, "Expected serial GPS source to open PTY slave") && ok;

    std::string sentence;
    ok = check(writeAll(pty.masterFd(),
                        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"),
               "Expected PTY write to succeed") &&
         ok;
    ok = check(source->nextSentence(sentence), "Expected serial GPS source to read one sentence") && ok;
    ok = check(sentence == "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
               "Expected serial GPS source to trim CR/LF") &&
         ok;
    return ok;
}

bool testSerialSourceSkipsBlankLines() {
    bool ok = true;
    PtyPair pty;
    std::unique_ptr<OBC::GPS::IGpsSentenceSource> source =
        OBC::GPS::makeSerialGpsSentenceSource(pty.slavePath(), 9600U, 50U);
    ok = check(source != nullptr, "Expected serial GPS source to open PTY slave") && ok;

    std::string sentence;
    ok = check(writeAll(pty.masterFd(),
                        "\r\n$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A\n"),
               "Expected PTY write with blank line to succeed") &&
         ok;
    ok = check(source->nextSentence(sentence), "Expected serial GPS source to skip blank line and read sentence") &&
         ok;
    ok = check(sentence == "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A",
               "Expected serial GPS source to ignore blank line noise") &&
         ok;
    return ok;
}

bool testSerialSourceBlankLineNoiseStillTimesOut() {
    bool ok = true;
    PtyPair pty;
    std::unique_ptr<OBC::GPS::IGpsSentenceSource> source =
        OBC::GPS::makeSerialGpsSentenceSource(pty.slavePath(), 9600U, 40U);
    ok = check(source != nullptr, "Expected serial GPS source to open PTY slave") && ok;

    std::thread noiseWriter([&pty]() {
        for (unsigned int index = 0; index < 200U; index++) {
            if (!writeAll(pty.masterFd(), "\r\n")) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    std::string sentence;
    const auto start = std::chrono::steady_clock::now();
    ok = check(!source->nextSentence(sentence),
               "Expected blank-line-only serial noise to resolve as bounded no-data") &&
         ok;
    const auto elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    ok = check(elapsedMs < 250, "Expected blank-line-only serial noise to return within bounded time") && ok;

    noiseWriter.join();
    return ok;
}

bool testSerialSourceTimeoutAndPartialLineRecovery() {
    bool ok = true;
    PtyPair pty;
    std::unique_ptr<OBC::GPS::IGpsSentenceSource> source =
        OBC::GPS::makeSerialGpsSentenceSource(pty.slavePath(), 9600U, 30U);
    ok = check(source != nullptr, "Expected serial GPS source to open PTY slave") && ok;

    std::string sentence;
    ok = check(!source->nextSentence(sentence), "Expected no-data timeout with empty PTY") && ok;

    ok = check(writeAll(pty.masterFd(), "$GPRMC,123519,A,4807.038"), "Expected partial PTY write to succeed") && ok;
    ok = check(!source->nextSentence(sentence), "Expected partial line without newline to time out") && ok;

    ok = check(writeAll(pty.masterFd(), ",N,01131.000,E,022.4,084.4,230394,003.1,W*6A\n"),
               "Expected completing PTY write to succeed") &&
         ok;
    ok = check(source->nextSentence(sentence), "Expected completed PTY line to be returned on next poll") && ok;
    ok = check(sentence == "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A",
               "Expected serial GPS source to preserve buffered partial line across timeout") &&
         ok;

    return ok;
}

}  // namespace

int main() {
    bool ok = true;
    ok = testParserHandlesValidGgaAndRmc() && ok;
    ok = testParserHandlesNoFixAndChecksumFailure() && ok;
    ok = testParserRejectsOversizedOrOverfieldSentence() && ok;
    ok = testReplaySourceCyclesLoadedSentences() && ok;
    ok = testSerialSourceTrimsCrLfAndReadsSentence() && ok;
    ok = testSerialSourceSkipsBlankLines() && ok;
    ok = testSerialSourceBlankLineNoiseStillTimesOut() && ok;
    ok = testSerialSourceTimeoutAndPartialLineRecovery() && ok;
    return ok ? 0 : 1;
}
