#include "OBC/Components/GpsBridge/GpsBridge.hpp"

#include <cstdlib>
#include <cerrno>
#include <limits>

#include "simulators/gps/NmeaParser.hpp"

namespace OBC {

namespace {

constexpr std::uint32_t GPS_DEFAULT_BAUDRATE = 9600U;
constexpr std::uint32_t GPS_SERIAL_TIMEOUT_MS = 250U;

bool parseSourceModeEnv(const char* raw, OBC::GpsSourceMode& mode) {
    if (raw == nullptr) {
        return false;
    }
    const std::string value(raw);
    if (value == "fake") {
        mode = OBC::GpsSourceMode::FAKE;
        return true;
    }
    if (value == "replay") {
        mode = OBC::GpsSourceMode::REPLAY;
        return true;
    }
    if (value == "live-uart") {
        mode = OBC::GpsSourceMode::LIVE_UART;
        return true;
    }
    return false;
}

std::uint32_t parseBaudrateEnv(const char* raw, const std::uint32_t fallback) {
    if (raw == nullptr || raw[0] == '\0') {
        return fallback;
    }

    char* end = nullptr;
    errno = 0;
    const unsigned long value = std::strtoul(raw, &end, 10);
    if (errno != 0 || end == raw || end == nullptr || *end != '\0' || value == 0UL ||
        value > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        return fallback;
    }
    return static_cast<std::uint32_t>(value);
}

OBC::GPS::SourceMode toRuntimeMode(const OBC::GpsSourceMode mode) {
    switch (mode.e) {
        case OBC::GpsSourceMode::LIVE_UART:
            return OBC::GPS::SourceMode::LIVE_UART;
        case OBC::GpsSourceMode::REPLAY:
            return OBC::GPS::SourceMode::REPLAY;
        case OBC::GpsSourceMode::FAKE:
        default:
            return OBC::GPS::SourceMode::FAKE;
    }
}

OBC::GpsSourceMode toTelemetryMode(const OBC::GPS::SourceMode mode) {
    switch (mode) {
        case OBC::GPS::SourceMode::LIVE_UART:
            return OBC::GpsSourceMode::LIVE_UART;
        case OBC::GPS::SourceMode::REPLAY:
            return OBC::GpsSourceMode::REPLAY;
        case OBC::GPS::SourceMode::FAKE:
        default:
            return OBC::GpsSourceMode::FAKE;
    }
}

}  // namespace

GpsBridge::GpsBridge(const char* const compName)
    : GpsBridgeComponentBase(compName),
      m_fakeSource(),
      m_replaySource(),
      m_liveSource(),
      m_testSource(),
      m_activeSource(nullptr),
      m_liveSerialDevice("/dev/serial0"),
      m_liveBaudrate(GPS_DEFAULT_BAUDRATE),
      m_cachedState(),
      m_fixLatched(false) {}

GpsBridge::~GpsBridge() = default;

void GpsBridge::configureRuntime(const std::string& runtimeRoot) {
    static_cast<void>(runtimeRoot);
    this->m_fakeSource = OBC::GPS::makeDefaultFakeGpsSentenceSource();
    this->m_liveSerialDevice = std::getenv("OBC_GPS_SERIAL_DEVICE") != nullptr &&
                                       std::getenv("OBC_GPS_SERIAL_DEVICE")[0] != '\0'
                                   ? std::getenv("OBC_GPS_SERIAL_DEVICE")
                                   : "/dev/serial0";
    this->m_liveBaudrate = parseBaudrateEnv(std::getenv("OBC_GPS_BAUDRATE"), GPS_DEFAULT_BAUDRATE);

    const char* replayPath = std::getenv("OBC_GPS_REPLAY_FILE");
    if (replayPath != nullptr && replayPath[0] != '\0') {
        this->m_replaySource = OBC::GPS::makeReplayFileGpsSentenceSource(replayPath);
    } else {
        this->m_replaySource.reset();
    }
    this->m_liveSource.reset();

    OBC::GpsSourceMode initialMode = OBC::GpsSourceMode::FAKE;
    static_cast<void>(parseSourceModeEnv(std::getenv("OBC_GPS_SOURCE_MODE"), initialMode));
    if (!this->activateSource_(initialMode)) {
        static_cast<void>(this->activateSource_(OBC::GpsSourceMode::FAKE));
    }
    this->publishSummaryTelemetry_(this->m_cachedState);
}

void GpsBridge::setSentenceSourceForTest(std::unique_ptr<OBC::GPS::IGpsSentenceSource> source, OBC::GpsSourceMode mode) {
    this->m_testSource = std::move(source);
    this->m_activeSource = this->m_testSource.get();
    this->m_cachedState.sourceMode = toRuntimeMode(mode);
    this->publishSummaryTelemetry_(this->m_cachedState);
}

bool GpsBridge::pollStateForTest() {
    return this->poll_(PollMode::SCHEDULED);
}

bool GpsBridge::getCachedStateForRuntime(OBC::GPS::StateData& state) const {
    state = this->m_cachedState;
    return this->m_cachedState.hasSample;
}

Fw::CmdResponse GpsBridge::setSourceModeForRuntime(OBC::GpsSourceMode mode) {
    if (!this->activateSource_(mode)) {
        return Fw::CmdResponse::VALIDATION_ERROR;
    }
    this->publishSummaryTelemetry_(this->m_cachedState);
    return Fw::CmdResponse::OK;
}

void GpsBridge::schedIn_handler(const FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    static_cast<void>(this->poll_(PollMode::SCHEDULED));
}

void GpsBridge::GPS_GET_STATE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    const bool success = this->poll_(PollMode::EXPLICIT_REFRESH);
    this->cmdResponse_out(opCode, cmdSeq, success ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR);
}

void GpsBridge::GPS_SET_SOURCE_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::GpsSourceMode mode) {
    this->cmdResponse_out(opCode, cmdSeq, this->setSourceModeForRuntime(mode));
}

bool GpsBridge::poll_(PollMode pollMode) {
    if (this->m_activeSource == nullptr) {
        this->log_WARNING_HI_GPS_SOURCE_ERROR(static_cast<U32>(SourceError::NOT_CONFIGURED));
        return false;
    }

    std::string sentence;
    if (!this->m_activeSource->nextSentence(sentence)) {
        this->log_WARNING_HI_GPS_SOURCE_ERROR(static_cast<U32>(SourceError::NO_SOURCE_DATA));
        return false;
    }

    OBC::GPS::SentenceUpdate update = {};
    const OBC::GPS::StateData previousState = this->m_cachedState;
    const OBC::GPS::ParseStatus status = OBC::GPS::parseNmeaSentence(sentence, update);
    switch (status) {
        case OBC::GPS::ParseStatus::OK:
            this->applyValidUpdate_(update);
            this->publishChangeDrivenTelemetry_(previousState, this->m_cachedState);
            if (pollMode == PollMode::EXPLICIT_REFRESH) {
                this->publishExplicitRefreshTelemetry_(this->m_cachedState);
                this->log_ACTIVITY_LO_GPS_STATE_UPDATED(1U, this->m_cachedState.satelliteCount);
            }
            return true;
        case OBC::GPS::ParseStatus::NO_FIX:
            this->applyNoFixUpdate_(update);
            this->publishChangeDrivenTelemetry_(previousState, this->m_cachedState);
            if (pollMode == PollMode::EXPLICIT_REFRESH) {
                this->publishExplicitRefreshTelemetry_(this->m_cachedState);
                this->log_ACTIVITY_LO_GPS_STATE_UPDATED(0U, this->m_cachedState.satelliteCount);
            }
            return true;
        case OBC::GPS::ParseStatus::UNSUPPORTED:
            return true;
        case OBC::GPS::ParseStatus::INVALID_CHECKSUM:
        case OBC::GPS::ParseStatus::MALFORMED:
        default:
            this->m_cachedState.rejectedSentenceCount++;
            if (pollMode == PollMode::EXPLICIT_REFRESH) {
                this->publishExplicitRefreshTelemetry_(this->m_cachedState);
            }
            this->log_WARNING_HI_GPS_PARSE_ERROR(static_cast<U32>(status));
            return false;
    }
}

bool GpsBridge::activateSource_(OBC::GpsSourceMode mode) {
    if (this->m_testSource != nullptr) {
        this->m_activeSource = this->m_testSource.get();
        this->m_cachedState.sourceMode = toRuntimeMode(mode);
        return true;
    }

    if (mode == OBC::GpsSourceMode::FAKE) {
        if (this->m_fakeSource == nullptr) {
            this->m_fakeSource = OBC::GPS::makeDefaultFakeGpsSentenceSource();
        }
        this->m_activeSource = this->m_fakeSource.get();
        this->m_cachedState.sourceMode = OBC::GPS::SourceMode::FAKE;
        if (this->m_activeSource != nullptr) {
            this->m_activeSource->reset();
            return true;
        }
        return false;
    }

    if (mode == OBC::GpsSourceMode::REPLAY) {
        if (this->m_replaySource == nullptr) {
            this->log_WARNING_HI_GPS_SOURCE_ERROR(static_cast<U32>(SourceError::REPLAY_NOT_AVAILABLE));
            return false;
        }
        this->m_activeSource = this->m_replaySource.get();
        this->m_cachedState.sourceMode = OBC::GPS::SourceMode::REPLAY;
        this->m_activeSource->reset();
        return true;
    }

    if (mode == OBC::GpsSourceMode::LIVE_UART) {
        if (this->m_liveSource == nullptr) {
            this->m_liveSource = OBC::GPS::makeSerialGpsSentenceSource(this->m_liveSerialDevice,
                                                                       this->m_liveBaudrate,
                                                                       GPS_SERIAL_TIMEOUT_MS);
        }
        if (this->m_liveSource == nullptr) {
            this->log_WARNING_HI_GPS_SOURCE_ERROR(static_cast<U32>(SourceError::LIVE_UART_NOT_AVAILABLE));
            return false;
        }
        this->m_activeSource = this->m_liveSource.get();
        this->m_cachedState.sourceMode = OBC::GPS::SourceMode::LIVE_UART;
        this->m_activeSource->reset();
        return true;
    }

    return false;
}

void GpsBridge::publishSummaryTelemetry_(const OBC::GPS::StateData& state) {
    this->tlmWrite_GPS_SOURCE_MODE(toTelemetryMode(state.sourceMode));
    this->tlmWrite_GPS_HAVE_SAMPLE(state.hasSample ? 1U : 0U);
    this->tlmWrite_GPS_FIX_VALID(state.fixValid ? 1U : 0U);
}

void GpsBridge::publishChangeDrivenTelemetry_(const OBC::GPS::StateData& previous, const OBC::GPS::StateData& current) {
    if (previous.sourceMode != current.sourceMode) {
        this->tlmWrite_GPS_SOURCE_MODE(toTelemetryMode(current.sourceMode));
    }
    if (previous.hasSample != current.hasSample) {
        this->tlmWrite_GPS_HAVE_SAMPLE(current.hasSample ? 1U : 0U);
    }
    if (previous.fixValid != current.fixValid) {
        this->tlmWrite_GPS_FIX_VALID(current.fixValid ? 1U : 0U);
    }
    if (previous.satelliteCount != current.satelliteCount) {
        this->tlmWrite_GPS_SAT_COUNT(current.satelliteCount);
    }
}

void GpsBridge::publishExplicitRefreshTelemetry_(const OBC::GPS::StateData& state) {
    this->tlmWrite_GPS_SOURCE_MODE(toTelemetryMode(state.sourceMode));
    this->tlmWrite_GPS_HAVE_SAMPLE(state.hasSample ? 1U : 0U);
    this->tlmWrite_GPS_FIX_VALID(state.fixValid ? 1U : 0U);
    this->tlmWrite_GPS_SAT_COUNT(state.satelliteCount);
    this->tlmWrite_GPS_LAT_DEG(state.latitudeDeg);
    this->tlmWrite_GPS_LON_DEG(state.longitudeDeg);
    this->tlmWrite_GPS_ALT_M(state.altitudeMeters);
    this->tlmWrite_GPS_SPEED_MPS(state.speedMetersPerSecond);
    this->tlmWrite_GPS_COURSE_DEG(state.courseDegrees);
    this->tlmWrite_GPS_HDOP(state.hdop);
    this->tlmWrite_GPS_UTC_SEC_OF_DAY(state.utcSecondsOfDay);
    this->tlmWrite_GPS_UTC_DATE_YMD(state.utcDateYmd);
    this->tlmWrite_GPS_ACCEPTED_SENTENCES(state.acceptedSentenceCount);
    this->tlmWrite_GPS_REJECTED_SENTENCES(state.rejectedSentenceCount);
}

void GpsBridge::applyValidUpdate_(const OBC::GPS::SentenceUpdate& update) {
    const bool hadValidFix = this->m_fixLatched;

    this->m_cachedState.hasSample = true;
    this->m_cachedState.fixValid = true;
    if (update.hasLatitudeLongitude) {
        this->m_cachedState.latitudeDeg = update.latitudeDeg;
        this->m_cachedState.longitudeDeg = update.longitudeDeg;
    }
    if (update.hasAltitude) {
        this->m_cachedState.altitudeMeters = update.altitudeMeters;
    }
    if (update.hasSpeed) {
        this->m_cachedState.speedMetersPerSecond = update.speedMetersPerSecond;
    }
    if (update.hasCourse) {
        this->m_cachedState.courseDegrees = update.courseDegrees;
    }
    if (update.hasSatelliteCount) {
        this->m_cachedState.satelliteCount = update.satelliteCount;
    }
    if (update.hasHdop) {
        this->m_cachedState.hdop = update.hdop;
    }
    if (update.hasUtcTime) {
        this->m_cachedState.utcSecondsOfDay = update.utcSecondsOfDay;
    }
    if (update.hasUtcDate) {
        this->m_cachedState.utcDateYmd = update.utcDateYmd;
    }
    this->m_cachedState.acceptedSentenceCount++;

    this->m_fixLatched = true;
    if (!hadValidFix) {
        this->log_ACTIVITY_HI_GPS_FIX_ACQUIRED(this->m_cachedState.latitudeDeg, this->m_cachedState.longitudeDeg);
    }
}

void GpsBridge::applyNoFixUpdate_(const OBC::GPS::SentenceUpdate& update) {
    const bool hadValidFix = this->m_fixLatched;

    this->m_cachedState.hasSample = true;
    this->m_cachedState.fixValid = false;
    this->m_cachedState.latitudeDeg = 0.0;
    this->m_cachedState.longitudeDeg = 0.0;
    this->m_cachedState.altitudeMeters = 0.0F;
    this->m_cachedState.speedMetersPerSecond = 0.0F;
    this->m_cachedState.courseDegrees = 0.0F;
    if (update.hasSatelliteCount) {
        this->m_cachedState.satelliteCount = update.satelliteCount;
    } else {
        this->m_cachedState.satelliteCount = 0U;
    }
    if (update.hasHdop) {
        this->m_cachedState.hdop = update.hdop;
    }
    if (update.hasUtcTime) {
        this->m_cachedState.utcSecondsOfDay = update.utcSecondsOfDay;
    }
    if (update.hasUtcDate) {
        this->m_cachedState.utcDateYmd = update.utcDateYmd;
    }
    this->m_cachedState.acceptedSentenceCount++;

    this->m_fixLatched = false;
    if (hadValidFix) {
        this->log_WARNING_LO_GPS_FIX_LOST();
    }
}

}  // namespace OBC
