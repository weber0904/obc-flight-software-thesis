#include "GpsBridgeTester.hpp"

#include <cstdlib>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <utility>

namespace OBC {

namespace {

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

}  // namespace

GpsBridgeTester::GpsBridgeTester()
    : GpsBridgeGTestBase("GpsBridgeTester", MAX_HISTORY_SIZE), component("GpsBridge") {
    this->initComponents();
    this->connectPorts();
}

GpsBridgeTester::~GpsBridgeTester() = default;

void GpsBridgeTester::testGetStatePublishesValidFix() {
    this->setScriptedSource_({
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
    });

    this->clearHistory();
    this->sendCmd_GPS_GET_STATE(TEST_INSTANCE_ID, 0);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GPS_GET_STATE, 0, Fw::CmdResponse::OK);
    ASSERT_EVENTS_GPS_FIX_ACQUIRED_SIZE(1);
    ASSERT_EVENTS_GPS_STATE_UPDATED_SIZE(1);
    ASSERT_EVENTS_GPS_STATE_UPDATED(0, 1U, 8U);
    ASSERT_EVENTS_GPS_PARSE_ERROR_SIZE(0);
    ASSERT_EVENTS_GPS_SOURCE_ERROR_SIZE(0);
    ASSERT_TLM_GPS_FIX_VALID_SIZE(2);
    ASSERT_TLM_GPS_FIX_VALID(1, 1U);
    ASSERT_TLM_GPS_HAVE_SAMPLE_SIZE(2);
    ASSERT_TLM_GPS_HAVE_SAMPLE(1, 1U);
    ASSERT_TLM_GPS_SAT_COUNT_SIZE(2);
    ASSERT_TLM_GPS_SAT_COUNT(1, 8U);
    ASSERT_TLM_GPS_LAT_DEG_SIZE(1);
    ASSERT_TLM_GPS_LAT_DEG(0, 48.1173);
    ASSERT_TLM_GPS_ACCEPTED_SENTENCES_SIZE(1);
    ASSERT_TLM_GPS_ACCEPTED_SENTENCES(0, 1U);
    ASSERT_TLM_GPS_REJECTED_SENTENCES_SIZE(1);
    ASSERT_TLM_GPS_REJECTED_SENTENCES(0, 0U);
}

void GpsBridgeTester::testGetStateReplaysTelemetryWhenValuesUnchanged() {
    this->setScriptedSource_({
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
    });

    this->sendCmd_GPS_GET_STATE(TEST_INSTANCE_ID, 0);
    this->clearHistory();

    this->sendCmd_GPS_GET_STATE(TEST_INSTANCE_ID, 1);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GPS_GET_STATE, 1, Fw::CmdResponse::OK);
    ASSERT_TLM_GPS_SOURCE_MODE_SIZE(1);
    ASSERT_TLM_GPS_SOURCE_MODE(0, OBC::GpsSourceMode::FAKE);
    ASSERT_TLM_GPS_FIX_VALID_SIZE(1);
    ASSERT_TLM_GPS_FIX_VALID(0, 1U);
    ASSERT_TLM_GPS_HAVE_SAMPLE_SIZE(1);
    ASSERT_TLM_GPS_HAVE_SAMPLE(0, 1U);
    ASSERT_TLM_GPS_LAT_DEG_SIZE(1);
    ASSERT_TLM_GPS_LAT_DEG(0, 48.1173);
    ASSERT_TLM_GPS_LON_DEG_SIZE(1);
    ASSERT_TLM_GPS_LON_DEG(0, 11.516666666666667);
    ASSERT_TLM_GPS_SAT_COUNT_SIZE(1);
    ASSERT_TLM_GPS_SAT_COUNT(0, 8U);
}

void GpsBridgeTester::testNoFixClearsLatchedFixViaScheduler() {
    this->setScriptedSource_({
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
        "$GPGGA,123520,,,,,0,00,99.99,,,,,,*4F",
    });

    this->sendCmd_GPS_GET_STATE(TEST_INSTANCE_ID, 0);
    this->clearHistory();

    this->invoke_to_schedIn(0, 0U);

    ASSERT_EVENTS_GPS_FIX_LOST_SIZE(1);
    ASSERT_EVENTS_GPS_STATE_UPDATED_SIZE(0);
    ASSERT_TLM_GPS_FIX_VALID_SIZE(1);
    ASSERT_TLM_GPS_FIX_VALID(0, 0U);
    ASSERT_TLM_GPS_SAT_COUNT_SIZE(1);
    ASSERT_TLM_GPS_SAT_COUNT(0, 0U);
    ASSERT_TLM_GPS_LAT_DEG_SIZE(0);
    ASSERT_TLM_GPS_ACCEPTED_SENTENCES_SIZE(0);

    OBC::GPS::StateData cached = {};
    ASSERT_TRUE(this->component.getCachedStateForRuntime(cached));
    ASSERT_EQ(cached.acceptedSentenceCount, 2U);
}

void GpsBridgeTester::testMalformedSentenceReportsParseError() {
    this->setScriptedSource_({
        "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00",
    });

    this->clearHistory();
    this->sendCmd_GPS_GET_STATE(TEST_INSTANCE_ID, 7);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GPS_GET_STATE, 7, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_GPS_PARSE_ERROR_SIZE(1);
    ASSERT_EVENTS_GPS_PARSE_ERROR(0, static_cast<U32>(OBC::GPS::ParseStatus::INVALID_CHECKSUM));
    ASSERT_EVENTS_GPS_STATE_UPDATED_SIZE(0);
    ASSERT_TLM_GPS_REJECTED_SENTENCES_SIZE(1);
    ASSERT_TLM_GPS_REJECTED_SENTENCES(0, 1U);
}

void GpsBridgeTester::testReplayModeRejectsWhenUnavailable() {
    ::unsetenv("OBC_GPS_REPLAY_FILE");
    ::unsetenv("OBC_GPS_SOURCE_MODE");

    this->component.configureRuntime("");
    this->clearHistory();

    this->sendCmd_GPS_SET_SOURCE_MODE(TEST_INSTANCE_ID, 9, OBC::GpsSourceMode::REPLAY);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GPS_SET_SOURCE_MODE, 9, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_GPS_SOURCE_ERROR_SIZE(1);
    ASSERT_EVENTS_GPS_SOURCE_ERROR(0, 3U);
    ASSERT_TLM_GPS_SOURCE_MODE_SIZE(0);
}

void GpsBridgeTester::testLiveUartModeRejectsWhenUnavailable() {
    ::unsetenv("OBC_GPS_REPLAY_FILE");
    ::unsetenv("OBC_GPS_SOURCE_MODE");
    ::setenv("OBC_GPS_SERIAL_DEVICE", "/definitely/missing-gps-tty", 1);
    ::setenv("OBC_GPS_BAUDRATE", "9600", 1);

    this->component.configureRuntime("");
    this->clearHistory();

    this->sendCmd_GPS_SET_SOURCE_MODE(TEST_INSTANCE_ID, 10, OBC::GpsSourceMode::LIVE_UART);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GPS_SET_SOURCE_MODE, 10, Fw::CmdResponse::VALIDATION_ERROR);
    ASSERT_EVENTS_GPS_SOURCE_ERROR_SIZE(1);
    ASSERT_EVENTS_GPS_SOURCE_ERROR(0, 4U);
    ASSERT_TLM_GPS_SOURCE_MODE_SIZE(0);

    ::unsetenv("OBC_GPS_SERIAL_DEVICE");
    ::unsetenv("OBC_GPS_BAUDRATE");
}

void GpsBridgeTester::testLiveUartModeFallsBackWhenBaudrateEnvOverflows() {
    PtyPair pty;
    ASSERT_TRUE(pty.valid());

    ::unsetenv("OBC_GPS_REPLAY_FILE");
    ::unsetenv("OBC_GPS_SOURCE_MODE");
    ::setenv("OBC_GPS_SERIAL_DEVICE", pty.slavePath().c_str(), 1);
    ::setenv("OBC_GPS_BAUDRATE", "999999999999999999999", 1);

    this->component.configureRuntime("");
    this->clearHistory();

    this->sendCmd_GPS_SET_SOURCE_MODE(TEST_INSTANCE_ID, 13, OBC::GpsSourceMode::LIVE_UART);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GPS_SET_SOURCE_MODE, 13, Fw::CmdResponse::OK);
    ASSERT_TLM_GPS_SOURCE_MODE_SIZE(1);
    ASSERT_TLM_GPS_SOURCE_MODE(0, OBC::GpsSourceMode::LIVE_UART);
    ASSERT_EVENTS_GPS_SOURCE_ERROR_SIZE(0);

    ::unsetenv("OBC_GPS_SERIAL_DEVICE");
    ::unsetenv("OBC_GPS_BAUDRATE");
}

void GpsBridgeTester::testLiveUartModePublishesTelemetryWithTestSource() {
    this->setScriptedSource_({
        "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47",
    });

    this->clearHistory();
    this->sendCmd_GPS_SET_SOURCE_MODE(TEST_INSTANCE_ID, 12, OBC::GpsSourceMode::LIVE_UART);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GPS_SET_SOURCE_MODE, 12, Fw::CmdResponse::OK);
    ASSERT_TLM_GPS_SOURCE_MODE_SIZE(1);
    ASSERT_TLM_GPS_SOURCE_MODE(0, OBC::GpsSourceMode::LIVE_UART);
    ASSERT_EVENTS_GPS_SOURCE_ERROR_SIZE(0);
}

void GpsBridgeTester::testSourceDepletionReportsSourceError() {
    this->setScriptedSource_({});

    this->clearHistory();
    this->sendCmd_GPS_GET_STATE(TEST_INSTANCE_ID, 11);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GPS_GET_STATE, 11, Fw::CmdResponse::EXECUTION_ERROR);
    ASSERT_EVENTS_GPS_SOURCE_ERROR_SIZE(1);
    ASSERT_EVENTS_GPS_SOURCE_ERROR(0, 2U);
    ASSERT_EVENTS_GPS_PARSE_ERROR_SIZE(0);
}

void GpsBridgeTester::setScriptedSource_(std::vector<std::string> sentences, OBC::GpsSourceMode mode) {
    this->component.setSentenceSourceForTest(
        std::unique_ptr<OBC::GPS::IGpsSentenceSource>(new ScriptedGpsSource(std::move(sentences))),
        mode
    );
}

}  // namespace OBC
