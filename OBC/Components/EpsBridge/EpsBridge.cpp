#include "OBC/Components/EpsBridge/EpsBridge.hpp"
#include "OBC/Types/WatchdogSourceEnumAc.hpp"

#include <cerrno>
#include <cstdlib>
#include <limits>

namespace OBC {

namespace {

constexpr U32 DEFAULT_SCHEDULED_POLL_PERIOD_TICKS = 1U;
constexpr U32 DEFAULT_OPERATOR_TLM_PERIOD_TICKS = 1U;

U32 readScheduledPollPeriodTicks(const char* name, U32 fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    if (errno == ERANGE || end == value || end == nullptr || *end != '\0' || parsed == 0ULL ||
        parsed > static_cast<unsigned long long>(std::numeric_limits<U32>::max())) {
        return fallback;
    }
    return static_cast<U32>(parsed);
}

OBC::EPS::TransportStatus mapAsyncRuntimeStatus(OBC::CSP::RuntimeStatus status) {
    switch (status) {
        case OBC::CSP::RuntimeStatus::OK:
            return OBC::EPS::TransportStatus::OK;
        case OBC::CSP::RuntimeStatus::INVALID_ARGUMENT:
            return OBC::EPS::TransportStatus::INVALID_REQUEST;
        case OBC::CSP::RuntimeStatus::TIMEOUT:
            return OBC::EPS::TransportStatus::TIMEOUT;
        case OBC::CSP::RuntimeStatus::EXECUTION_ERROR:
        default:
            return OBC::EPS::TransportStatus::TRANSPORT_ERROR;
    }
}

}  // namespace

EpsBridge::EpsBridge(const char* const compName, U32 timeoutMs)
    : EpsBridgeComponentBase(compName),
      m_timeoutMs(timeoutMs),
      m_asyncRuntimeOwner(nullptr),
      m_cspTargetNode(OBC::EPS::defaultEpsNodeIdFromEnvironment()),
      m_asyncSeq(1U),
      m_pollPeriodTicks(readScheduledPollPeriodTicks("EPS_SCHEDULED_POLL_PERIOD_TICKS",
                                                     DEFAULT_SCHEDULED_POLL_PERIOD_TICKS)),
      m_ticksUntilNextPoll(0U),
      m_operatorTlmPeriodTicks(readScheduledPollPeriodTicks("EPS_OPERATOR_TLM_PERIOD_TICKS",
                                                            DEFAULT_OPERATOR_TLM_PERIOD_TICKS)),
      m_ticksUntilOperatorTlm(0U),
      m_asyncScheduledPoll(),
      m_ownedTransport(OBC::EPS::makeDefaultEpsTransport(timeoutMs)),
      m_transport(this->m_ownedTransport.get()),
      m_hasValidStatus(false),
      m_lastStatus(),
      m_pollHealth(),
      m_lastPduStatus(0U),
      m_lowBatteryLatched(false),
      m_criticalBatteryLatched(false),
      m_overtempLatched(false) {}

EpsBridge::~EpsBridge() = default;

void EpsBridge::setTransportForTest(OBC::EPS::IEpsTransport* transport) {
    this->m_transport = transport;
}

bool EpsBridge::pollStatusForTest() {
    OBC::EPS::StatusData status = {};
    const OBC::EPS::TransportStatus result = this->requestStatus_(status);
    if (result != OBC::EPS::TransportStatus::OK) {
        this->emitCommError_(1U);
        this->invalidateCachedStatus_();
        this->notePollFailure_();
        return false;
    }

    this->updateCachedStatus_(status, true);
    if (!this->m_explicitRefresh.active && this->shouldEmitScheduledOperatorTelemetry_()) {
        this->publishScheduledOperatorTelemetry_(status);
    }
    this->notePollSuccess_();
    return true;
}

void EpsBridge::tickScheduledPollForTest() {
    this->schedIn_handler(0, 0U);
}

bool EpsBridge::getStatusForRuntime(OBC::EPS::StatusData& status) {
    const OBC::EPS::TransportStatus result = this->requestStatus_(status);
    if (result != OBC::EPS::TransportStatus::OK) {
        this->emitCommError_(1U);
        this->invalidateCachedStatus_();
        return false;
    }
    this->updateCachedStatus_(status, false);
    this->publishExplicitRefreshTelemetry_(status);
    return true;
}

bool EpsBridge::getCachedStatusForRuntime(OBC::EPS::StatusData& status) const {
    if (!this->m_hasValidStatus) {
        return false;
    }

    status = this->m_lastStatus;
    return true;
}

bool EpsBridge::getPollHealthForRuntime(OBC::EPS::PollHealthState& state) const {
    state = this->m_pollHealth;
    return true;
}

void EpsBridge::configureCspRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime) {
    this->m_ownedTransport = OBC::EPS::makeDefaultEpsTransport(runtime, this->m_timeoutMs);
    this->m_transport = this->m_ownedTransport.get();
    this->m_asyncRuntimeOwner = dynamic_cast<OBC::IAsyncCspRuntimeOwner*>(&runtime);
    this->m_asyncScheduledPoll = {};
    this->m_ticksUntilNextPoll = 0U;
}

void EpsBridge::configureScheduledPollPeriodForTest(U32 periodTicks) {
    this->m_pollPeriodTicks = periodTicks == 0U ? 1U : periodTicks;
    this->m_ticksUntilNextPoll = 0U;
}

void EpsBridge::configureOperatorTelemetryPeriodForTest(U32 periodTicks) {
    this->m_operatorTlmPeriodTicks = periodTicks == 0U ? 1U : periodTicks;
    this->m_ticksUntilOperatorTlm = 0U;
}

#ifdef BUILD_UT
void EpsBridge::setPollHealthForTest(const OBC::EPS::PollHealthState& state) {
    this->m_pollHealth = state;
}

void EpsBridge::drainExplicitRefreshForTest(std::uint8_t maxFields) {
    this->drainExplicitRefresh_(maxFields);
}
#endif

Fw::CmdResponse EpsBridge::setPduForRuntime(U8 channel, bool enabled, OBC::EPS::StatusData& status) {
    const OBC::EPS::TransportStatus result = this->requestSetPdu_(channel, enabled, status);
    if (result == OBC::EPS::TransportStatus::OK) {
        this->updateCachedStatus_(status, false);
        this->publishExplicitRefreshTelemetry_(status);
    } else {
        this->emitCommError_(2U);
    }
    return this->mapTransportStatus_(result);
}

Fw::CmdResponse EpsBridge::setHeaterForRuntime(bool enabled, OBC::EPS::StatusData& status) {
    const OBC::EPS::TransportStatus result = this->requestSetHeater_(enabled, status);
    if (result == OBC::EPS::TransportStatus::OK) {
        this->updateCachedStatus_(status, false);
        this->publishExplicitRefreshTelemetry_(status);
    } else {
        this->emitCommError_(3U);
    }
    return this->mapTransportStatus_(result);
}

Fw::CmdResponse EpsBridge::resetForRuntime(OBC::EPS::StatusData& status) {
    const OBC::EPS::TransportStatus result = this->requestReset_(status);
    if (result == OBC::EPS::TransportStatus::OK) {
        this->updateCachedStatus_(status, false);
        this->publishExplicitRefreshTelemetry_(status);
    } else {
        this->emitCommError_(4U);
    }
    return this->mapTransportStatus_(result);
}

void EpsBridge::schedIn_handler(const FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    this->drainExplicitRefresh_(EXPLICIT_REFRESH_FIELDS_PER_TICK);
    static_cast<void>(this->pollScheduledStatus_());
    if (this->isConnected_watchdogBeatOut_OutputPort(0)) {
        this->watchdogBeatOut_out(0, static_cast<U32>(OBC::WatchdogSource::EPS_BRIDGE));
    }
}

void EpsBridge::EPS_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    OBC::EPS::StatusData status = {};
    this->cmdResponse_out(opCode, cmdSeq, this->getStatusForRuntime(status) ? Fw::CmdResponse::OK
                                                                            : Fw::CmdResponse::EXECUTION_ERROR);
}

void EpsBridge::EPS_SET_PDU_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 channel, bool enabled) {
    OBC::EPS::StatusData status = {};
    this->cmdResponse_out(opCode, cmdSeq, this->setPduForRuntime(channel, enabled, status));
}

void EpsBridge::EPS_SET_HEATER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enabled) {
    OBC::EPS::StatusData status = {};
    this->cmdResponse_out(opCode, cmdSeq, this->setHeaterForRuntime(enabled, status));
}

void EpsBridge::EPS_RESET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    OBC::EPS::StatusData status = {};
    this->cmdResponse_out(opCode, cmdSeq, this->resetForRuntime(status));
}

OBC::EPS::TransportStatus EpsBridge::requestStatus_(OBC::EPS::StatusData& status) {
    return this->m_transport->getStatus(status);
}

OBC::EPS::TransportStatus EpsBridge::requestSetPdu_(U8 channel, bool enabled, OBC::EPS::StatusData& status) {
    return this->m_transport->setPdu(channel, enabled, status);
}

OBC::EPS::TransportStatus EpsBridge::requestSetHeater_(bool enabled, OBC::EPS::StatusData& status) {
    return this->m_transport->setHeater(enabled, status);
}

OBC::EPS::TransportStatus EpsBridge::requestReset_(OBC::EPS::StatusData& status) {
    return this->m_transport->reset(status);
}

bool EpsBridge::pollScheduledStatus_() {
    if (this->m_asyncRuntimeOwner == nullptr) {
        if (this->m_ticksUntilNextPoll > 0U) {
            this->m_ticksUntilNextPoll -= 1U;
            return this->m_hasValidStatus;
        }
        this->m_ticksUntilNextPoll = this->m_pollPeriodTicks > 0U ? (this->m_pollPeriodTicks - 1U) : 0U;
        return this->pollStatusForTest();
    }

    const bool updated = this->consumeAsyncScheduledStatus_();
    if (this->m_ticksUntilNextPoll > 0U) {
        this->m_ticksUntilNextPoll -= 1U;
        return updated || this->m_hasValidStatus;
    }

    this->m_ticksUntilNextPoll = this->m_pollPeriodTicks > 0U ? (this->m_pollPeriodTicks - 1U) : 0U;
    if (!this->m_asyncScheduledPoll.inFlight) {
        return this->submitAsyncScheduledStatus_() || updated;
    }

    this->m_asyncRuntimeOwner->recordCoalescedForRuntime();
    return updated || this->m_hasValidStatus;
}

bool EpsBridge::consumeAsyncScheduledStatus_() {
    if (!this->m_asyncScheduledPoll.inFlight) {
        return false;
    }

    OBC::AsyncCspRequestReplyCompletion completion = {};
    if (!this->m_asyncRuntimeOwner->takeAsyncRequestReplyCompletion(this->m_asyncScheduledPoll.handle, completion)) {
        return false;
    }

    const OBC::EPS::CSP::Request request =
        OBC::EPS::CSP::makeBlankRequest(OBC::EPS::CSP::ServicePort::STATUS, this->m_asyncScheduledPoll.seq);
    this->m_asyncScheduledPoll = {};

    OBC::EPS::StatusData status = {};
    const OBC::EPS::TransportStatus result =
        completion.status == OBC::CSP::RuntimeStatus::OK
            ? OBC::EPS::decodeEpsReply(
                  request, OBC::EPS::CSP::ServicePort::STATUS, completion.reply.data(), completion.replySize, status)
            : mapAsyncRuntimeStatus(completion.status);
    if (result != OBC::EPS::TransportStatus::OK) {
        this->emitCommError_(1U);
        this->invalidateCachedStatus_();
        this->notePollFailure_();
        return false;
    }

    this->updateCachedStatus_(status, true);
    if (!this->m_explicitRefresh.active && this->shouldEmitScheduledOperatorTelemetry_()) {
        this->publishScheduledOperatorTelemetry_(status);
    }
    this->notePollSuccess_();
    return true;
}

bool EpsBridge::submitAsyncScheduledStatus_() {
    const std::uint16_t seq = this->nextAsyncSeq_();
    const OBC::EPS::CSP::Request request = OBC::EPS::CSP::makeBlankRequest(OBC::EPS::CSP::ServicePort::STATUS, seq);
    std::uint64_t handle = 0U;
    if (!this->m_asyncRuntimeOwner->submitAsyncRequestReply(this->m_cspTargetNode,
                                                            static_cast<std::uint8_t>(OBC::EPS::CSP::ServicePort::STATUS),
                                                            &request,
                                                            sizeof(request),
                                                            sizeof(OBC::EPS::CSP::Reply),
                                                            this->m_timeoutMs,
                                                            handle)) {
        this->emitCommError_(1U);
        this->invalidateCachedStatus_();
        this->notePollFailure_();
        return false;
    }

    this->m_asyncScheduledPoll.inFlight = true;
    this->m_asyncScheduledPoll.handle = handle;
    this->m_asyncScheduledPoll.seq = seq;
    return true;
}

std::uint16_t EpsBridge::nextAsyncSeq_() {
    const std::uint16_t current = this->m_asyncSeq;
    this->m_asyncSeq = static_cast<std::uint16_t>(this->m_asyncSeq + 1U);
    if (this->m_asyncSeq == 0U) {
        this->m_asyncSeq = 1U;
    }
    return current;
}

bool EpsBridge::shouldEmitScheduledOperatorTelemetry_() {
    if (this->m_ticksUntilOperatorTlm > 0U) {
        this->m_ticksUntilOperatorTlm -= 1U;
        return false;
    }
    this->m_ticksUntilOperatorTlm = this->m_operatorTlmPeriodTicks > 0U ? (this->m_operatorTlmPeriodTicks - 1U) : 0U;
    return true;
}

void EpsBridge::updateCachedStatus_(const OBC::EPS::StatusData& status, bool emitChangeDrivenTelemetry) {
    if (emitChangeDrivenTelemetry) {
        this->publishChangeDrivenTelemetry_(status);
    }

    if (this->m_hasValidStatus && this->m_lastPduStatus != status.pdu_status) {
        this->log_ACTIVITY_HI_EPS_PDU_CHANGE(status.pdu_status);
    }

    if (status.soc < CRITICAL_BATTERY_SOC) {
        if (!this->m_criticalBatteryLatched) {
            this->log_WARNING_HI_EPS_CRITICAL_BATTERY(status.soc);
            this->m_criticalBatteryLatched = true;
        }
    } else {
        this->m_criticalBatteryLatched = false;
    }

    if (status.soc < LOW_BATTERY_SOC) {
        if (!this->m_lowBatteryLatched) {
            this->log_WARNING_HI_EPS_LOW_BATTERY(status.soc);
            this->m_lowBatteryLatched = true;
        }
    } else {
        this->m_lowBatteryLatched = false;
    }

    if (status.temp_bat > OVERTEMP_THRESHOLD_C) {
        if (!this->m_overtempLatched) {
            this->log_WARNING_HI_EPS_OVERTEMP(status.temp_bat, OVERTEMP_THRESHOLD_C);
            this->m_overtempLatched = true;
        }
    } else {
        this->m_overtempLatched = false;
    }

    this->m_hasValidStatus = true;
    this->m_lastStatus = status;
    this->m_lastPduStatus = status.pdu_status;
    this->m_pollHealth.cacheValid = true;
}

void EpsBridge::publishScheduledOperatorTelemetry_(const OBC::EPS::StatusData& status) {
    this->tlmWrite_EPS_VBAT(status.vbat);
    this->tlmWrite_EPS_IBAT(status.ibat);
    this->tlmWrite_EPS_SOC(status.soc);
    this->tlmWrite_EPS_TEMP_BAT(status.temp_bat);
}

void EpsBridge::publishExplicitRefreshTelemetry_(const OBC::EPS::StatusData& status) {
    this->log_ACTIVITY_LO_EPS_STATUS_RECEIVED();
    this->queueExplicitRefresh_(status);
    this->drainExplicitRefresh_(EXPLICIT_REFRESH_FIELD_COUNT);
}

void EpsBridge::publishChangeDrivenTelemetry_(const OBC::EPS::StatusData& status) {
    if (!this->m_hasValidStatus) {
        return;
    }
    if (this->m_lastStatus.pdu_status != status.pdu_status) {
        this->tlmWrite_EPS_PDU_STATUS(status.pdu_status);
    }
    const bool previousHeaterEnabled = this->m_lastStatus.heater_enabled != 0U;
    const bool currentHeaterEnabled = status.heater_enabled != 0U;
    if (previousHeaterEnabled != currentHeaterEnabled) {
        this->tlmWrite_EPS_HEATER_ENABLED(currentHeaterEnabled);
    }
    if (this->m_lastStatus.overcurrent_flags != status.overcurrent_flags) {
        this->tlmWrite_EPS_OVERCURRENT_FLAGS(status.overcurrent_flags);
    }
}

void EpsBridge::emitStatusRefreshTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag) {
    this->epsStatusRefreshTlmOut_out(0, this->getIdBase() + channelId, timeTag, buffer);
}

void EpsBridge::queueExplicitRefresh_(const OBC::EPS::StatusData& status) {
    this->m_explicitRefresh.active = true;
    this->m_explicitRefresh.nextField = 0U;
    this->m_explicitRefresh.status = status;
    this->m_ticksUntilOperatorTlm = this->m_operatorTlmPeriodTicks;
}

void EpsBridge::drainExplicitRefresh_(std::uint8_t maxFields) {
    if (!this->m_explicitRefresh.active) {
        return;
    }

    Fw::Time timeTag = this->getTime();
    std::uint8_t emitted = 0U;
    while (this->m_explicitRefresh.active && emitted < maxFields) {
        this->emitExplicitRefreshField_(this->m_explicitRefresh.nextField, this->m_explicitRefresh.status, timeTag);
        this->m_explicitRefresh.nextField =
            static_cast<std::uint8_t>(this->m_explicitRefresh.nextField + 1U);
        emitted = static_cast<std::uint8_t>(emitted + 1U);
        if (this->m_explicitRefresh.nextField >= EXPLICIT_REFRESH_FIELD_COUNT) {
            this->m_explicitRefresh.active = false;
        }
    }
}

void EpsBridge::emitExplicitRefreshField_(std::uint8_t fieldIndex,
                                          const OBC::EPS::StatusData& status,
                                          Fw::Time& timeTag) {
    switch (fieldIndex) {
        case 0U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_VBAT, status.vbat, timeTag);
            break;
        case 1U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_IBAT, status.ibat, timeTag);
            break;
        case 2U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_SOC, status.soc, timeTag);
            break;
        case 3U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_TEMP_BAT, status.temp_bat, timeTag);
            break;
        case 4U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_PDU_STATUS, status.pdu_status, timeTag);
            break;
        case 5U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_HEATER_ENABLED, status.heater_enabled != 0U, timeTag);
            break;
        case 6U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_OVERCURRENT_FLAGS, status.overcurrent_flags, timeTag);
            break;
        case 7U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_VSOLAR, status.vsolar, timeTag);
            break;
        case 8U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_ISOLAR, status.isolar, timeTag);
            break;
        case 9U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_EPS_POWER_OUT, status.power_out, timeTag);
            break;
        default:
            FW_ASSERT(0, static_cast<FwAssertArgType>(fieldIndex));
            break;
    }
}

void EpsBridge::invalidateCachedStatus_() {
    this->m_hasValidStatus = false;
    this->m_pollHealth.cacheValid = false;
}

void EpsBridge::notePollSuccess_() {
    this->m_pollHealth.cacheValid = this->m_hasValidStatus;
    this->m_pollHealth.lastPollSucceeded = true;
    this->m_pollHealth.consecutivePollFailures = 0U;
}

void EpsBridge::notePollFailure_() {
    this->m_pollHealth.cacheValid = false;
    this->m_pollHealth.lastPollSucceeded = false;
    saturatingIncrement_(this->m_pollHealth.consecutivePollFailures);
    saturatingIncrement_(this->m_pollHealth.cumulativePollErrors);
}

void EpsBridge::saturatingIncrement_(U32& value) {
    if (value < std::numeric_limits<U32>::max()) {
        value++;
    }
}

void EpsBridge::emitCommError_(U32 code) {
    this->log_WARNING_HI_EPS_COMM_ERROR(code);
}

Fw::CmdResponse EpsBridge::mapTransportStatus_(OBC::EPS::TransportStatus status) const {
    if (status == OBC::EPS::TransportStatus::OK) {
        return Fw::CmdResponse::OK;
    }

    if (status == OBC::EPS::TransportStatus::INVALID_REQUEST) {
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    return Fw::CmdResponse::EXECUTION_ERROR;
}

}  // namespace OBC
