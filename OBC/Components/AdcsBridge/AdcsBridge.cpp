#include "OBC/Components/AdcsBridge/AdcsBridge.hpp"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace OBC {

namespace {

constexpr std::uint8_t ADCS_MODE_DETUMBLE = 1U;
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

F32 omegaNorm(const OBC::ADCS::StateData& state) {
    return std::sqrt(state.omega_x * state.omega_x + state.omega_y * state.omega_y + state.omega_z * state.omega_z);
}

void saturatingIncrement(U32& value) {
    if (value < std::numeric_limits<U32>::max()) {
        value++;
    }
}

OBC::ADCS::TransportStatus mapAsyncRuntimeStatus(OBC::CSP::RuntimeStatus status) {
    switch (status) {
        case OBC::CSP::RuntimeStatus::OK:
            return OBC::ADCS::TransportStatus::OK;
        case OBC::CSP::RuntimeStatus::INVALID_ARGUMENT:
            return OBC::ADCS::TransportStatus::INVALID_REQUEST;
        case OBC::CSP::RuntimeStatus::TIMEOUT:
            return OBC::ADCS::TransportStatus::TIMEOUT;
        case OBC::CSP::RuntimeStatus::EXECUTION_ERROR:
        default:
            return OBC::ADCS::TransportStatus::TRANSPORT_ERROR;
    }
}

}  // namespace

AdcsBridge::AdcsBridge(const char* const compName, U32 timeoutMs)
    : AdcsBridgeComponentBase(compName),
      m_timeoutMs(timeoutMs),
      m_asyncRuntimeOwner(nullptr),
      m_cspTargetNode(OBC::ADCS::defaultAdcsNodeIdFromEnvironment()),
      m_asyncSeq(1U),
      m_pollPeriodTicks(readScheduledPollPeriodTicks("ADCS_SCHEDULED_POLL_PERIOD_TICKS",
                                                     DEFAULT_SCHEDULED_POLL_PERIOD_TICKS)),
      m_ticksUntilNextPoll(0U),
      m_operatorTlmPeriodTicks(readScheduledPollPeriodTicks("ADCS_OPERATOR_TLM_PERIOD_TICKS",
                                                            DEFAULT_OPERATOR_TLM_PERIOD_TICKS)),
      m_ticksUntilOperatorTlm(0U),
      m_asyncScheduledPoll(),
      m_ownedTransport(OBC::ADCS::makeDefaultAdcsTransport(timeoutMs)),
      m_transport(this->m_ownedTransport.get()),
      m_hasValidState(false),
      m_cachedState(),
      m_explicitRefresh(),
      m_pollHealth(),
      m_lastMode(OBC::AdcsMode::IDLE),
      m_detumbleLatched(false),
      m_pointingLatched(false) {}

AdcsBridge::~AdcsBridge() = default;

void AdcsBridge::setTransportForTest(OBC::ADCS::IAdcsTransport* transport) {
    this->m_transport = transport;
}

bool AdcsBridge::pollStateForTest() {
    return this->pollScheduledState_();
}

void AdcsBridge::tickScheduledPollForTest() {
    this->schedIn_handler(0, 0U);
}

bool AdcsBridge::getStateForRuntime(OBC::ADCS::StateData& state) {
    const OBC::ADCS::TransportStatus result = this->requestState_(state);
    if (result != OBC::ADCS::TransportStatus::OK) {
        this->emitCommError_(1U);
        return false;
    }
    if (!this->updateCachedState_(state, false)) {
        return false;
    }
    this->publishExplicitRefreshTelemetry_(state);
    return true;
}

bool AdcsBridge::getCachedStateForRuntime(OBC::ADCS::StateData& state) const {
    if (!this->m_hasValidState) {
        return false;
    }

    state = this->m_cachedState;
    return true;
}

bool AdcsBridge::getPollHealthForRuntime(OBC::ADCS::PollHealthState& state) const {
    state = this->m_pollHealth;
    return true;
}

void AdcsBridge::configureCspRuntimeForRuntime(OBC::CSP::ICspRuntime& runtime) {
    this->m_ownedTransport = OBC::ADCS::makeDefaultAdcsTransport(runtime, this->m_timeoutMs);
    this->m_transport = this->m_ownedTransport.get();
    this->m_asyncRuntimeOwner = dynamic_cast<OBC::IAsyncCspRuntimeOwner*>(&runtime);
    this->m_asyncScheduledPoll = {};
    this->m_ticksUntilNextPoll = 0U;
    this->m_ticksUntilOperatorTlm = 0U;
}

void AdcsBridge::configureOperatorTelemetryPeriodForTest(U32 periodTicks) {
    this->m_operatorTlmPeriodTicks = periodTicks == 0U ? 1U : periodTicks;
    this->m_ticksUntilOperatorTlm = 0U;
}

void AdcsBridge::configureScheduledPollPeriodForTest(U32 periodTicks) {
    this->m_pollPeriodTicks = periodTicks == 0U ? 1U : periodTicks;
    this->m_ticksUntilNextPoll = 0U;
}

Fw::CmdResponse AdcsBridge::setModeForRuntime(OBC::AdcsMode mode, OBC::ADCS::StateData& state) {
    const OBC::ADCS::TransportStatus result = this->requestSetMode_(mode, state);
    Fw::CmdResponse response = this->mapTransportStatus_(result);
    if (result == OBC::ADCS::TransportStatus::OK) {
        if (!this->updateCachedState_(state, true)) {
            response = Fw::CmdResponse::EXECUTION_ERROR;
        } else {
            this->publishExplicitRefreshTelemetry_(state);
        }
    } else {
        this->emitCommError_(2U);
    }
    return response;
}

Fw::CmdResponse AdcsBridge::setTargetForRuntime(F64 q0, F64 q1, F64 q2, F64 q3, OBC::ADCS::StateData& state) {
    const OBC::ADCS::TransportStatus result = this->requestSetTarget_(q0, q1, q2, q3, state);
    Fw::CmdResponse response = this->mapTransportStatus_(result);
    if (result == OBC::ADCS::TransportStatus::OK) {
        if (!this->updateCachedState_(state, true)) {
            response = Fw::CmdResponse::EXECUTION_ERROR;
        } else {
            this->publishExplicitRefreshTelemetry_(state);
        }
    } else {
        this->emitCommError_(3U);
    }
    return response;
}

Fw::CmdResponse AdcsBridge::calibrateForRuntime(U8 sensorId, OBC::ADCS::StateData& state) {
    const OBC::ADCS::TransportStatus result = this->requestCalibrate_(sensorId, state);
    Fw::CmdResponse response = this->mapTransportStatus_(result);
    if (result == OBC::ADCS::TransportStatus::OK) {
        if (!this->updateCachedState_(state, true)) {
            response = Fw::CmdResponse::EXECUTION_ERROR;
        } else {
            this->publishExplicitRefreshTelemetry_(state);
        }
    } else {
        this->emitCommError_(4U);
    }
    return response;
}

Fw::CmdResponse AdcsBridge::resetForRuntime(OBC::ADCS::StateData& state) {
    const OBC::ADCS::TransportStatus result = this->requestReset_(state);
    Fw::CmdResponse response = this->mapTransportStatus_(result);
    if (result == OBC::ADCS::TransportStatus::OK) {
        if (!this->updateCachedState_(state, true)) {
            response = Fw::CmdResponse::EXECUTION_ERROR;
        } else {
            this->publishExplicitRefreshTelemetry_(state);
        }
    } else {
        this->emitCommError_(5U);
    }
    return response;
}

bool AdcsBridge::commandDetumbleForRuntime() {
    if (this->m_hasValidState && this->m_cachedState.mode == ADCS_MODE_DETUMBLE) {
        return true;
    }

    OBC::ADCS::StateData state = {};
    return this->setModeForRuntime(OBC::AdcsMode::DETUMBLE, state) == Fw::CmdResponse::OK;
}

bool AdcsBridge::commandSunSafePointingForRuntime(double q0, double q1, double q2, double q3) {
    OBC::ADCS::StateData state = {};
    if (this->setModeForRuntime(OBC::AdcsMode::POINTING, state) != Fw::CmdResponse::OK) {
        return false;
    }

    return this->setTargetForRuntime(q0, q1, q2, q3, state) == Fw::CmdResponse::OK;
}

bool AdcsBridge::requestPointingForTtcEntry() {
    OBC::ADCS::StateData state = {};
    return this->setModeForRuntime(OBC::AdcsMode::POINTING, state) == Fw::CmdResponse::OK;
}

void AdcsBridge::schedIn_handler(const FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    this->drainExplicitRefresh_(EXPLICIT_REFRESH_FIELDS_PER_TICK);
    static_cast<void>(this->pollScheduledStateAsync_());
}

void AdcsBridge::ADCS_SET_MODE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, OBC::AdcsMode mode) {
    OBC::ADCS::StateData state = {};
    this->cmdResponse_out(opCode, cmdSeq, this->setModeForRuntime(mode, state));
}

void AdcsBridge::ADCS_SET_TARGET_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F64 q0, F64 q1, F64 q2, F64 q3) {
    OBC::ADCS::StateData state = {};
    this->cmdResponse_out(opCode, cmdSeq, this->setTargetForRuntime(q0, q1, q2, q3, state));
}

void AdcsBridge::ADCS_GET_ATTITUDE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    OBC::ADCS::StateData state = {};
    this->cmdResponse_out(opCode, cmdSeq, this->getStateForRuntime(state) ? Fw::CmdResponse::OK
                                                                          : Fw::CmdResponse::EXECUTION_ERROR);
}

void AdcsBridge::ADCS_CALIBRATE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 sensorId) {
    OBC::ADCS::StateData state = {};
    this->cmdResponse_out(opCode, cmdSeq, this->calibrateForRuntime(sensorId, state));
}

bool AdcsBridge::pollScheduledState_() {
    OBC::ADCS::StateData state = {};
    const OBC::ADCS::TransportStatus result = this->requestState_(state);
    if (result != OBC::ADCS::TransportStatus::OK) {
        this->emitCommError_(1U);
        this->noteScheduledTransportFailure_();
        return false;
    }

    if (this->updateCachedState_(state, true)) {
        if (!this->m_explicitRefresh.active && this->shouldEmitScheduledOperatorTelemetry_()) {
            this->publishScheduledOperatorTelemetry_(state);
        }
        this->noteScheduledValidRefresh_();
        return true;
    }
    this->noteScheduledNoValidRefresh_();
    return false;
}

bool AdcsBridge::pollScheduledStateAsync_() {
    if (this->m_asyncRuntimeOwner == nullptr) {
        if (this->m_ticksUntilNextPoll > 0U) {
            this->m_ticksUntilNextPoll -= 1U;
            return this->m_hasValidState;
        }
        this->m_ticksUntilNextPoll = this->m_pollPeriodTicks > 0U ? (this->m_pollPeriodTicks - 1U) : 0U;
        return this->pollScheduledState_();
    }

    const bool updated = this->consumeAsyncScheduledState_();
    if (this->m_ticksUntilNextPoll > 0U) {
        this->m_ticksUntilNextPoll -= 1U;
        return updated || this->m_hasValidState;
    }

    this->m_ticksUntilNextPoll = this->m_pollPeriodTicks > 0U ? (this->m_pollPeriodTicks - 1U) : 0U;
    if (!this->m_asyncScheduledPoll.inFlight) {
        return this->submitAsyncScheduledState_() || updated;
    }

    this->m_asyncRuntimeOwner->recordCoalescedForRuntime();
    return updated || this->m_hasValidState;
}

bool AdcsBridge::consumeAsyncScheduledState_() {
    if (!this->m_asyncScheduledPoll.inFlight) {
        return false;
    }

    OBC::AsyncCspRequestReplyCompletion completion = {};
    if (!this->m_asyncRuntimeOwner->takeAsyncRequestReplyCompletion(this->m_asyncScheduledPoll.handle, completion)) {
        return false;
    }

    const OBC::ADCS::CSP::Request request =
        OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::STATE, this->m_asyncScheduledPoll.seq);
    this->m_asyncScheduledPoll = {};

    OBC::ADCS::StateData state = {};
    const OBC::ADCS::TransportStatus result =
        completion.status == OBC::CSP::RuntimeStatus::OK
            ? OBC::ADCS::decodeAdcsReply(
                  request, OBC::ADCS::CSP::ServicePort::STATE, completion.reply.data(), completion.replySize, state)
            : mapAsyncRuntimeStatus(completion.status);
    if (result != OBC::ADCS::TransportStatus::OK) {
        this->emitCommError_(1U);
        this->noteScheduledTransportFailure_();
        return false;
    }

    if (this->updateCachedState_(state, true)) {
        if (!this->m_explicitRefresh.active && this->shouldEmitScheduledOperatorTelemetry_()) {
            this->publishScheduledOperatorTelemetry_(state);
        }
        this->noteScheduledValidRefresh_();
        return true;
    }

    this->noteScheduledNoValidRefresh_();
    return false;
}

bool AdcsBridge::submitAsyncScheduledState_() {
    const std::uint16_t seq = this->nextAsyncSeq_();
    const OBC::ADCS::CSP::Request request = OBC::ADCS::CSP::makeBlankRequest(OBC::ADCS::CSP::ServicePort::STATE, seq);
    std::uint64_t handle = 0U;
    if (!this->m_asyncRuntimeOwner->submitAsyncRequestReply(this->m_cspTargetNode,
                                                            static_cast<std::uint8_t>(OBC::ADCS::CSP::ServicePort::STATE),
                                                            &request,
                                                            sizeof(request),
                                                            sizeof(OBC::ADCS::CSP::Reply),
                                                            this->m_timeoutMs,
                                                            handle)) {
        this->emitCommError_(1U);
        this->noteScheduledTransportFailure_();
        return false;
    }

    this->m_asyncScheduledPoll.inFlight = true;
    this->m_asyncScheduledPoll.handle = handle;
    this->m_asyncScheduledPoll.seq = seq;
    return true;
}

std::uint16_t AdcsBridge::nextAsyncSeq_() {
    const std::uint16_t current = this->m_asyncSeq;
    this->m_asyncSeq = static_cast<std::uint16_t>(this->m_asyncSeq + 1U);
    if (this->m_asyncSeq == 0U) {
        this->m_asyncSeq = 1U;
    }
    return current;
}

OBC::ADCS::TransportStatus AdcsBridge::requestState_(OBC::ADCS::StateData& state) {
    return this->m_transport->getState(state);
}

OBC::ADCS::TransportStatus AdcsBridge::requestSetMode_(OBC::AdcsMode mode, OBC::ADCS::StateData& state) {
    return this->m_transport->setMode(static_cast<U8>(mode.e), state);
}

OBC::ADCS::TransportStatus AdcsBridge::requestSetTarget_(F64 q0, F64 q1, F64 q2, F64 q3, OBC::ADCS::StateData& state) {
    return this->m_transport->setTarget(q0, q1, q2, q3, state);
}

OBC::ADCS::TransportStatus AdcsBridge::requestCalibrate_(U8 sensorId, OBC::ADCS::StateData& state) {
    return this->m_transport->calibrate(sensorId, state);
}

OBC::ADCS::TransportStatus AdcsBridge::requestReset_(OBC::ADCS::StateData& state) {
    return this->m_transport->reset(state);
}

bool AdcsBridge::shouldEmitScheduledOperatorTelemetry_() {
    if (this->m_ticksUntilOperatorTlm > 0U) {
        this->m_ticksUntilOperatorTlm -= 1U;
        return false;
    }
    this->m_ticksUntilOperatorTlm = this->m_operatorTlmPeriodTicks > 0U ? (this->m_operatorTlmPeriodTicks - 1U) : 0U;
    return true;
}

bool AdcsBridge::updateCachedState_(const OBC::ADCS::StateData& state, bool emitChangeDrivenTelemetry) {
    if (state.sensor_valid == 0U) {
        this->log_WARNING_HI_ADCS_SENSOR_FAULT(1U);
        return false;
    }

    const OBC::AdcsMode mode(static_cast<OBC::AdcsMode::T>(state.mode));
    const F32 rateNorm = omegaNorm(state);

    if (emitChangeDrivenTelemetry) {
        this->publishChangeDrivenTelemetry_(mode);
    }

    if (this->m_hasValidState && this->m_lastMode != mode) {
        this->log_ACTIVITY_HI_ADCS_MODE_CHANGE(mode);
    }

    if (mode == OBC::AdcsMode::DETUMBLE && rateNorm < DETUMBLE_THRESHOLD) {
        if (!this->m_detumbleLatched) {
            this->log_ACTIVITY_HI_ADCS_DETUMBLE_COMPLETE(rateNorm);
            this->m_detumbleLatched = true;
        }
    } else {
        this->m_detumbleLatched = false;
    }

    if (mode == OBC::AdcsMode::POINTING && state.pointing_error_deg < POINTING_THRESHOLD_DEG) {
        if (!this->m_pointingLatched) {
            this->log_ACTIVITY_HI_ADCS_POINTING_ACQUIRED(state.pointing_error_deg);
            this->m_pointingLatched = true;
        }
    } else {
        this->m_pointingLatched = false;
    }

    this->m_cachedState = state;
    this->m_hasValidState = true;
    this->m_lastMode = mode;
    return true;
}

void AdcsBridge::publishScheduledOperatorTelemetry_(const OBC::ADCS::StateData& state) {
    this->tlmWrite_ADCS_Q0(state.q0);
    this->tlmWrite_ADCS_Q1(state.q1);
    this->tlmWrite_ADCS_Q2(state.q2);
    this->tlmWrite_ADCS_Q3(state.q3);
    this->tlmWrite_ADCS_OMEGA_X(state.omega_x);
    this->tlmWrite_ADCS_OMEGA_Y(state.omega_y);
    this->tlmWrite_ADCS_OMEGA_Z(state.omega_z);
}

void AdcsBridge::publishExplicitRefreshTelemetry_(const OBC::ADCS::StateData& state) {
    this->queueExplicitRefresh_(state);
    this->drainExplicitRefresh_(EXPLICIT_REFRESH_FIELD_COUNT);
}

void AdcsBridge::publishChangeDrivenTelemetry_(OBC::AdcsMode mode) {
    if (!this->m_hasValidState) {
        return;
    }
    if (this->m_lastMode != mode) {
        this->tlmWrite_ADCS_MODE(mode);
    }
}

void AdcsBridge::emitStatusRefreshTelemetry_(FwChanIdType channelId, Fw::TlmBuffer& buffer, Fw::Time& timeTag) {
    this->adcsStatusRefreshTlmOut_out(0, this->getIdBase() + channelId, timeTag, buffer);
}

void AdcsBridge::queueExplicitRefresh_(const OBC::ADCS::StateData& state) {
    this->m_explicitRefresh.active = true;
    this->m_explicitRefresh.nextField = 0U;
    this->m_explicitRefresh.state = state;
    this->m_ticksUntilOperatorTlm = this->m_operatorTlmPeriodTicks;
}

void AdcsBridge::drainExplicitRefresh_(std::uint8_t maxFields) {
    if (!this->m_explicitRefresh.active) {
        return;
    }

    Fw::Time timeTag = this->getTime();
    std::uint8_t emitted = 0U;
    while (this->m_explicitRefresh.active && emitted < maxFields) {
        this->emitExplicitRefreshField_(this->m_explicitRefresh.nextField, this->m_explicitRefresh.state, timeTag);
        this->m_explicitRefresh.nextField = static_cast<std::uint8_t>(this->m_explicitRefresh.nextField + 1U);
        emitted = static_cast<std::uint8_t>(emitted + 1U);
        if (this->m_explicitRefresh.nextField >= EXPLICIT_REFRESH_FIELD_COUNT) {
            this->m_explicitRefresh.active = false;
        }
    }
}

void AdcsBridge::emitExplicitRefreshField_(std::uint8_t fieldIndex,
                                           const OBC::ADCS::StateData& state,
                                           Fw::Time& timeTag) {
    switch (fieldIndex) {
        case 0U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_MODE,
                                                   OBC::AdcsMode(static_cast<OBC::AdcsMode::T>(state.mode)),
                                                   timeTag);
            break;
        case 1U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_Q0, state.q0, timeTag);
            break;
        case 2U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_Q1, state.q1, timeTag);
            break;
        case 3U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_Q2, state.q2, timeTag);
            break;
        case 4U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_Q3, state.q3, timeTag);
            break;
        case 5U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_OMEGA_X, state.omega_x, timeTag);
            break;
        case 6U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_OMEGA_Y, state.omega_y, timeTag);
            break;
        case 7U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_OMEGA_Z, state.omega_z, timeTag);
            break;
        case 8U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_MAG_X, state.mag_x, timeTag);
            break;
        case 9U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_MAG_Y, state.mag_y, timeTag);
            break;
        case 10U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_MAG_Z, state.mag_z, timeTag);
            break;
        case 11U:
            this->emitStatusRefreshTelemetryValue_(CHANNELID_ADCS_POINTING_ERR, state.pointing_error_deg, timeTag);
            break;
        default:
            break;
    }
}

void AdcsBridge::noteScheduledTransportFailure_() {
    this->m_pollHealth.lastScheduledTransportOk = false;
    this->m_pollHealth.lastScheduledValidRefresh = false;
    this->m_pollHealth.hasValidState = this->m_hasValidState;
    saturatingIncrement(this->m_pollHealth.consecutiveTransportFailures);
    this->m_pollHealth.consecutiveNoValidRefresh = 0U;
    saturatingIncrement(this->m_pollHealth.cumulativeTransportErrors);
}

void AdcsBridge::noteScheduledNoValidRefresh_() {
    this->m_pollHealth.lastScheduledTransportOk = true;
    this->m_pollHealth.lastScheduledValidRefresh = false;
    this->m_pollHealth.hasValidState = this->m_hasValidState;
    this->m_pollHealth.consecutiveTransportFailures = 0U;
    saturatingIncrement(this->m_pollHealth.consecutiveNoValidRefresh);
    saturatingIncrement(this->m_pollHealth.cumulativeNoValidRefresh);
}

void AdcsBridge::noteScheduledValidRefresh_() {
    this->m_pollHealth.lastScheduledTransportOk = true;
    this->m_pollHealth.lastScheduledValidRefresh = true;
    this->m_pollHealth.hasValidState = this->m_hasValidState;
    this->m_pollHealth.consecutiveTransportFailures = 0U;
    this->m_pollHealth.consecutiveNoValidRefresh = 0U;
}

void AdcsBridge::emitCommError_(U32 code) {
    this->log_WARNING_HI_ADCS_COMM_ERROR(code);
}

Fw::CmdResponse AdcsBridge::mapTransportStatus_(OBC::ADCS::TransportStatus status) const {
    if (status == OBC::ADCS::TransportStatus::OK) {
        return Fw::CmdResponse::OK;
    }

    if (status == OBC::ADCS::TransportStatus::INVALID_REQUEST) {
        return Fw::CmdResponse::VALIDATION_ERROR;
    }

    return Fw::CmdResponse::EXECUTION_ERROR;
}

}  // namespace OBC
