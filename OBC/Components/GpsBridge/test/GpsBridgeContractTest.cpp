#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "OBC/Components/GpsBridge/GpsBridge.hpp"
#include "simulators/gps/GpsSource.hpp"

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

class ScriptedGpsSource final : public OBC::GPS::IGpsSentenceSource {
  public:
    explicit ScriptedGpsSource(std::vector<std::string> sentences)
        : m_sentences(std::move(sentences)), m_index(0U) {}

    bool nextSentence(std::string& sentence) override {
        if (this->m_index >= this->m_sentences.size()) {
            return false;
        }
        sentence = this->m_sentences[this->m_index++];
        return true;
    }

    void reset() override {
        this->m_index = 0U;
    }

  private:
    std::vector<std::string> m_sentences;
    std::size_t m_index;
};

class PtyPair {
  public:
    PtyPair() : m_masterFd(-1) {
        this->m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
        if (this->m_masterFd < 0) {
            return;
        }
        if (::grantpt(this->m_masterFd) != 0 || ::unlockpt(this->m_masterFd) != 0) {
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
            return;
        }
        char* slave = ::ptsname(this->m_masterFd);
        this->m_slavePath = slave != nullptr ? slave : "";
    }

    ~PtyPair() {
        if (this->m_masterFd >= 0) {
            ::close(this->m_masterFd);
            this->m_masterFd = -1;
        }
    }

    bool valid() const {
        return this->m_masterFd >= 0 && !this->m_slavePath.empty();
    }

    const std::string& slavePath() const {
        return this->m_slavePath;
    }

  private:
    int m_masterFd;
    std::string m_slavePath;
};

bool testReplayModeRejectsWhenUnavailable() {
    bool ok = true;

    ::unsetenv("OBC_GPS_REPLAY_FILE");
    ::unsetenv("OBC_GPS_SOURCE_MODE");

    OBC::GpsBridge bridge("gpsBridgeReplayContract");
    bridge.configureRuntime("");

    OBC::GPS::StateData state = {};
    static_cast<void>(bridge.getCachedStateForRuntime(state));
    ok = check(state.sourceMode == OBC::GPS::SourceMode::FAKE, "Expected fake mode after default runtime configure") &&
         ok;

    ok = check(bridge.setSourceModeForRuntime(OBC::GpsSourceMode::REPLAY) == Fw::CmdResponse::VALIDATION_ERROR,
               "Expected replay source mode change to fail when replay source is unavailable") &&
         ok;

    static_cast<void>(bridge.getCachedStateForRuntime(state));
    ok = check(state.sourceMode == OBC::GPS::SourceMode::FAKE,
               "Expected bridge to remain in fake mode after rejected replay switch") &&
         ok;

    return ok;
}

bool testLiveUartModeRejectsWhenUnavailable() {
    bool ok = true;

    ::unsetenv("OBC_GPS_REPLAY_FILE");
    ::unsetenv("OBC_GPS_SOURCE_MODE");
    ::setenv("OBC_GPS_SERIAL_DEVICE", "/definitely/missing-gps-tty", 1);
    ::setenv("OBC_GPS_BAUDRATE", "9600", 1);

    OBC::GpsBridge bridge("gpsBridgeLiveUartContract");
    bridge.configureRuntime("");

    OBC::GPS::StateData state = {};
    static_cast<void>(bridge.getCachedStateForRuntime(state));
    ok = check(state.sourceMode == OBC::GPS::SourceMode::FAKE, "Expected fake mode after default runtime configure") &&
         ok;

    ok = check(bridge.setSourceModeForRuntime(OBC::GpsSourceMode::LIVE_UART) == Fw::CmdResponse::VALIDATION_ERROR,
               "Expected live-uart source mode change to fail when serial source is unavailable") &&
         ok;

    static_cast<void>(bridge.getCachedStateForRuntime(state));
    ok = check(state.sourceMode == OBC::GPS::SourceMode::FAKE,
               "Expected bridge to remain in fake mode after rejected live-uart switch") &&
         ok;

    ::unsetenv("OBC_GPS_SERIAL_DEVICE");
    ::unsetenv("OBC_GPS_BAUDRATE");
    return ok;
}

bool testLiveUartModeFallsBackWhenBaudrateEnvOverflows() {
    bool ok = true;
    PtyPair pty;
    ok = check(pty.valid(), "Expected PTY pair for live-uart fallback contract test") && ok;

    ::unsetenv("OBC_GPS_REPLAY_FILE");
    ::unsetenv("OBC_GPS_SOURCE_MODE");
    ::setenv("OBC_GPS_SERIAL_DEVICE", pty.slavePath().c_str(), 1);
    ::setenv("OBC_GPS_BAUDRATE", "999999999999999999999", 1);

    OBC::GpsBridge bridge("gpsBridgeLiveUartOverflowContract");
    bridge.configureRuntime("");

    OBC::GPS::StateData state = {};
    static_cast<void>(bridge.getCachedStateForRuntime(state));
    ok = check(state.sourceMode == OBC::GPS::SourceMode::FAKE, "Expected fake mode after default runtime configure") &&
         ok;

    ok = check(bridge.setSourceModeForRuntime(OBC::GpsSourceMode::LIVE_UART) == Fw::CmdResponse::OK,
               "Expected overflow baudrate env to fall back to the default supported live-uart baudrate") &&
         ok;

    static_cast<void>(bridge.getCachedStateForRuntime(state));
    ok = check(state.sourceMode == OBC::GPS::SourceMode::LIVE_UART,
               "Expected bridge to enter live-uart mode after fallback baudrate parsing") &&
         ok;

    ::unsetenv("OBC_GPS_SERIAL_DEVICE");
    ::unsetenv("OBC_GPS_BAUDRATE");
    return ok;
}

bool testBridgeContractTracksStateAcrossValidNoFixRejectedAndModeChanges() {
    bool ok = true;

    OBC::GpsBridge bridge("gpsBridgeStateContract");
    bridge.setSentenceSourceForTest(
        std::unique_ptr<OBC::GPS::IGpsSentenceSource>(new ScriptedGpsSource({
            "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
            "$GPGGA,123520,,,,,0,00,99.99,,,,,,*4F",
            "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00",
        })),
        OBC::GpsSourceMode::FAKE);

    OBC::GPS::StateData state = {};
    ok = check(bridge.pollStateForTest(), "Expected valid GPS sentence to succeed") && ok;
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached GPS state after valid sample") && ok;
    ok = check(state.fixValid, "Expected valid fix after valid GPS sentence") && ok;
    ok = check(state.satelliteCount == 8U, "Expected satellite count propagated from valid GPS sentence") && ok;
    ok = check(state.acceptedSentenceCount == 1U, "Expected one accepted sentence after first poll") && ok;
    ok = check(state.rejectedSentenceCount == 0U, "Expected zero rejected sentences after first poll") && ok;

    ok = check(bridge.pollStateForTest(), "Expected no-fix GPS sentence to succeed") && ok;
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached GPS state after no-fix sample") && ok;
    ok = check(!state.fixValid, "Expected no-fix sentence to clear the valid-fix state") && ok;
    ok = check(state.acceptedSentenceCount == 2U, "Expected accepted sentence count to advance on no-fix sample") &&
         ok;
    ok = check(state.rejectedSentenceCount == 0U, "Expected rejected count to stay zero after no-fix sample") &&
         ok;

    ok = check(!bridge.pollStateForTest(), "Expected malformed GPS sentence to be rejected") && ok;
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached GPS state to remain readable after reject") &&
         ok;
    ok = check(state.acceptedSentenceCount == 2U, "Expected accepted count unchanged after rejected sentence") && ok;
    ok = check(state.rejectedSentenceCount == 1U, "Expected rejected count incremented after malformed sentence") &&
         ok;

    ok = check(bridge.setSourceModeForRuntime(OBC::GpsSourceMode::REPLAY) == Fw::CmdResponse::OK,
               "Expected test source to honor replay runtime mode switching for contract tests") &&
         ok;
    ok = check(bridge.getCachedStateForRuntime(state), "Expected cached GPS state after runtime mode switch") && ok;
    ok = check(state.sourceMode == OBC::GPS::SourceMode::REPLAY,
               "Expected runtime source mode to update even while test source remains active") &&
         ok;

    ok = check(bridge.setSourceModeForRuntime(OBC::GpsSourceMode::LIVE_UART) == Fw::CmdResponse::OK,
               "Expected test source to honor live-uart runtime mode switching for contract tests") &&
         ok;
    ok = check(bridge.getCachedStateForRuntime(state),
               "Expected cached GPS state after runtime live-uart mode switch") &&
         ok;
    ok = check(state.sourceMode == OBC::GPS::SourceMode::LIVE_UART,
               "Expected runtime source mode to update to live-uart while test source remains active") &&
         ok;

    return ok;
}

}  // namespace

int main() {
    bool ok = true;
    ok = testReplayModeRejectsWhenUnavailable() && ok;
    ok = testLiveUartModeRejectsWhenUnavailable() && ok;
    ok = testLiveUartModeFallsBackWhenBaudrateEnvOverflows() && ok;
    ok = testBridgeContractTracksStateAcrossValidNoFixRejectedAndModeChanges() && ok;
    return ok ? 0 : 1;
}
