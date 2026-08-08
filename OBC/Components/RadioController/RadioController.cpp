#include "OBC/Components/RadioController/RadioController.hpp"

#include <limits>

namespace OBC {

RadioController::RadioController(const char* const compName)
    : RadioControllerComponentBase(compName),
      m_transport(nullptr),
      m_haveStatus(false),
      m_lastEnabled(false),
      m_statusAgeTicks(0U),
      m_lastStatus(),
      m_lastObservationResult(OBC::RadioObservationResult::NO_SAMPLE) {}

RadioController::~RadioController() = default;

void RadioController::configureTransport(std::unique_ptr<OBC::COMM::IRadioTransport> transport) {
    this->m_ownedTransport = std::move(transport);
    this->m_transport = this->m_ownedTransport.get();
    this->resetObservation_();
}

void RadioController::setTransportForTest(OBC::COMM::IRadioTransport* transport) {
    this->m_ownedTransport.reset();
    this->m_transport = transport;
    this->resetObservation_();
}

bool RadioController::pollStatusForTest() {
    if (this->m_transport == nullptr) {
        this->updateObservationResult_(OBC::COMM::RadioTransportStatus::TRANSPORT_ERROR);
        return false;
    }

    OBC::COMM::RadioStatus status = {};
    const OBC::COMM::RadioTransportStatus result = this->m_transport->getStatus(status);
    if (result != OBC::COMM::RadioTransportStatus::OK) {
        this->updateObservationResult_(result);
        return false;
    }

    return this->applyStatus_(status, PublishMode::SUMMARY);
}

bool RadioController::getStatusForRuntime(OBC::COMM::RadioStatus& status) {
    if (this->m_transport == nullptr) {
        this->updateObservationResult_(OBC::COMM::RadioTransportStatus::TRANSPORT_ERROR);
        return false;
    }
    const OBC::COMM::RadioTransportStatus result = this->m_transport->getStatus(status);
    if (result != OBC::COMM::RadioTransportStatus::OK) {
        this->updateObservationResult_(result);
        return false;
    }
    return this->applyStatus_(status, PublishMode::DETAILED);
}

bool RadioController::getCachedStatusForRuntime(OBC::COMM::RadioStatus& status) const {
    if (!this->m_haveStatus) {
        return false;
    }
    status = this->m_lastStatus;
    return true;
}

OBC::RadioObservationState RadioController::getObservationForRuntime() const {
    OBC::RadioObservationState observation = {};
    observation.haveSample = this->m_haveStatus;
    observation.statusAgeTicks = this->m_statusAgeTicks;
    observation.lastResult = this->m_lastObservationResult;
    observation.lastStatus = this->m_lastStatus;
    return observation;
}

Fw::CmdResponse RadioController::enableForRuntime(bool enabled, OBC::COMM::RadioStatus& status) {
    if (this->m_transport == nullptr) {
        this->updateObservationResult_(OBC::COMM::RadioTransportStatus::TRANSPORT_ERROR);
        return Fw::CmdResponse::EXECUTION_ERROR;
    }
    const OBC::COMM::RadioTransportStatus result = this->m_transport->setEnabled(enabled, status);
    if (result != OBC::COMM::RadioTransportStatus::OK) {
        this->updateObservationResult_(result);
        return this->mapStatus_(result);
    }
    if (!this->applyStatus_(status, PublishMode::DETAILED)) {
        return this->mapStatus_(result);
    }
    return Fw::CmdResponse::OK;
}

Fw::CmdResponse RadioController::setPowerForRuntime(U8 powerDbm, OBC::COMM::RadioStatus& status) {
    if (this->m_transport == nullptr) {
        this->updateObservationResult_(OBC::COMM::RadioTransportStatus::TRANSPORT_ERROR);
        return Fw::CmdResponse::EXECUTION_ERROR;
    }
    const OBC::COMM::RadioTransportStatus result = this->m_transport->setPower(powerDbm, status);
    if (result != OBC::COMM::RadioTransportStatus::OK) {
        this->updateObservationResult_(result);
        return this->mapStatus_(result);
    }
    if (!this->applyStatus_(status, PublishMode::DETAILED)) {
        return this->mapStatus_(result);
    }
    return Fw::CmdResponse::OK;
}

Fw::CmdResponse RadioController::setFrequencyForRuntime(U32 freqHz, OBC::COMM::RadioStatus& status) {
    if (this->m_transport == nullptr) {
        this->updateObservationResult_(OBC::COMM::RadioTransportStatus::TRANSPORT_ERROR);
        return Fw::CmdResponse::EXECUTION_ERROR;
    }
    const OBC::COMM::RadioTransportStatus result = this->m_transport->setFrequency(freqHz, status);
    if (result != OBC::COMM::RadioTransportStatus::OK) {
        this->updateObservationResult_(result);
        return this->mapStatus_(result);
    }
    if (!this->applyStatus_(status, PublishMode::DETAILED)) {
        return this->mapStatus_(result);
    }
    return Fw::CmdResponse::OK;
}

OBC::COMM::ByteStreamStats RadioController::getLinkStatsForRuntime() const {
    if (this->m_transport == nullptr) {
        return OBC::COMM::ByteStreamStats{0U, 0U, 0U, 0U, false};
    }
    return this->m_transport->getLinkStats();
}

void RadioController::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    if (!this->pollStatusForTest() && this->m_haveStatus && this->m_statusAgeTicks < std::numeric_limits<U32>::max()) {
        this->m_statusAgeTicks += 1U;
        this->tlmWrite_RADIO_STATUS_SAMPLE_AVAILABLE(true);
        this->tlmWrite_RADIO_STATUS_AGE_TICKS(this->m_statusAgeTicks);
    }
}

void RadioController::RADIO_ENABLE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, bool enable) {
    OBC::COMM::RadioStatus status = {};
    this->cmdResponse_out(opCode, cmdSeq, this->enableForRuntime(enable, status));
}

void RadioController::RADIO_SET_POWER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U8 powerDbm) {
    OBC::COMM::RadioStatus status = {};
    this->cmdResponse_out(opCode, cmdSeq, this->setPowerForRuntime(powerDbm, status));
}

void RadioController::RADIO_SET_FREQ_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, U32 freqHz) {
    OBC::COMM::RadioStatus status = {};
    this->cmdResponse_out(opCode, cmdSeq, this->setFrequencyForRuntime(freqHz, status));
}

void RadioController::RADIO_GET_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    OBC::COMM::RadioStatus status = {};
    const bool ok = this->getStatusForRuntime(status);
    this->cmdResponse_out(opCode, cmdSeq, ok ? Fw::CmdResponse::OK : Fw::CmdResponse::EXECUTION_ERROR);
}

bool RadioController::applyStatus_(const OBC::COMM::RadioStatus& status, PublishMode publishMode) {
    if (this->m_haveStatus && status.enabled != this->m_lastEnabled) {
        if (status.enabled) {
            this->log_ACTIVITY_HI_RADIO_POWERED_ON();
        } else {
            this->log_ACTIVITY_HI_RADIO_POWERED_OFF();
        }
    }

    if (status.temperatureC > OVERTEMP_THRESHOLD_C) {
        this->log_WARNING_HI_RADIO_OVERTEMP(status.temperatureC);
    }

    if (publishMode == PublishMode::DETAILED) {
        this->publishDetailedStatus_(status);
    } else {
        this->publishSummaryStatus_(status);
    }

    this->m_haveStatus = true;
    this->m_lastEnabled = status.enabled;
    this->m_statusAgeTicks = 0U;
    this->m_lastStatus = status;
    this->m_lastObservationResult = OBC::RadioObservationResult::OK;
    return true;
}

void RadioController::publishSummaryStatus_(const OBC::COMM::RadioStatus& status) {
    this->tlmWrite_RADIO_ENABLED(status.enabled);
    this->tlmWrite_RADIO_STATUS_SAMPLE_AVAILABLE(true);
    this->tlmWrite_RADIO_STATUS_AGE_TICKS(0U);
    this->tlmWrite_RADIO_STATUS_RESULT(static_cast<U32>(OBC::RadioObservationResult::OK));
}

void RadioController::publishDetailedStatus_(const OBC::COMM::RadioStatus& status) {
    this->publishSummaryStatus_(status);
    this->tlmWrite_RADIO_TX_POWER(status.powerDbm);
    this->tlmWrite_RADIO_FREQ(status.freqHz);
    this->tlmWrite_RADIO_TEMP(status.temperatureC);
    this->tlmWrite_RADIO_RSSI(status.rssiDbm);
}

void RadioController::updateObservationResult_(OBC::COMM::RadioTransportStatus status) {
    const OBC::RadioObservationResult result = this->mapObservationResult_(status);
    this->m_lastObservationResult = result;
    this->tlmWrite_RADIO_STATUS_SAMPLE_AVAILABLE(this->m_haveStatus);
    this->tlmWrite_RADIO_STATUS_AGE_TICKS(this->m_statusAgeTicks);
    this->tlmWrite_RADIO_STATUS_RESULT(static_cast<U32>(result));
}

void RadioController::resetObservation_() {
    this->m_haveStatus = false;
    this->m_lastEnabled = false;
    this->m_statusAgeTicks = 0U;
    this->m_lastStatus = {};
    this->m_lastObservationResult = OBC::RadioObservationResult::NO_SAMPLE;
}

OBC::RadioObservationResult RadioController::mapObservationResult_(OBC::COMM::RadioTransportStatus status) const {
    switch (status) {
        case OBC::COMM::RadioTransportStatus::OK:
            return OBC::RadioObservationResult::OK;
        case OBC::COMM::RadioTransportStatus::TIMEOUT:
            return OBC::RadioObservationResult::TIMEOUT;
        case OBC::COMM::RadioTransportStatus::INVALID_RESPONSE:
            return OBC::RadioObservationResult::INVALID_RESPONSE;
        case OBC::COMM::RadioTransportStatus::UNSUPPORTED:
            return OBC::RadioObservationResult::UNSUPPORTED;
        case OBC::COMM::RadioTransportStatus::TRANSPORT_ERROR:
        default:
            return OBC::RadioObservationResult::TRANSPORT_ERROR;
    }
}

Fw::CmdResponse RadioController::mapStatus_(OBC::COMM::RadioTransportStatus status) const {
    switch (status) {
        case OBC::COMM::RadioTransportStatus::INVALID_RESPONSE:
            return Fw::CmdResponse::VALIDATION_ERROR;
        case OBC::COMM::RadioTransportStatus::UNSUPPORTED:
        case OBC::COMM::RadioTransportStatus::TIMEOUT:
        case OBC::COMM::RadioTransportStatus::TRANSPORT_ERROR:
        default:
            return Fw::CmdResponse::EXECUTION_ERROR;
    }
}

}  // namespace OBC
