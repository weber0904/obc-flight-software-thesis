#include "OBC/Components/AdcsFdirController/AdcsFdirController.hpp"

#include <limits>

#include "OBC/Types/WatchdogSourceEnumAc.hpp"

namespace OBC {

AdcsFdirController::AdcsFdirController(const char* const compName)
    : AdcsFdirControllerComponentBase(compName),
      m_healthProvider(nullptr),
      m_recoverySink(nullptr),
      m_faultLatched(false),
      m_latchedSource(OBC::RecoveryIncidentSource::NONE),
      m_lastFailureCount(0U),
      m_escalationCount(0U),
      m_recoveryCount(0U) {}

AdcsFdirController::~AdcsFdirController() = default;

void AdcsFdirController::configureRuntime(const OBC::IAdcsFdirHealthProvider* healthProvider,
                                          OBC::IRecoveryRequestSink* recoverySink) {
    this->m_healthProvider = healthProvider;
    this->m_recoverySink = recoverySink;
}

OBC::AdcsFdirDecision AdcsFdirController::runCycle() {
    if (this->m_healthProvider == nullptr) {
        return {};
    }

    OBC::ADCS::PollHealthState health = {};
    if (!this->m_healthProvider->getPollHealthForRuntime(health)) {
        return {};
    }

    const OBC::AdcsFdirDecision decision =
        OBC::AdcsFdirPolicy::evaluate(health, this->m_faultLatched, this->m_latchedSource);

    if (decision.shouldEnterRetry && decision.failureCount != this->m_lastFailureCount) {
        this->log_WARNING_LO_ADCS_FDIR_RETRYING(decision.activeSource, decision.failureCount);
    }

    if (decision.shouldLatchFault) {
        this->m_faultLatched = true;
        this->m_latchedSource = decision.activeSource;
        saturatingIncrement_(this->m_escalationCount);
        this->log_WARNING_HI_ADCS_FDIR_FAULT_ENTERED(decision.activeSource, decision.failureCount);
        this->submitFault_(decision);
    } else if (decision.shouldClearFault) {
        this->log_ACTIVITY_HI_ADCS_FDIR_FAULT_CLEARED(this->m_latchedSource, this->m_lastFailureCount);
        this->clearFault_();
        saturatingIncrement_(this->m_recoveryCount);
    }

    if (decision.shouldClearFault) {
        this->m_lastFailureCount = 0U;
    } else if (decision.shouldEnterRetry || decision.shouldLatchFault || decision.failureCount > 0U) {
        this->m_lastFailureCount = decision.failureCount;
    }
    this->publishState_(decision);
    return decision;
}

void AdcsFdirController::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    static_cast<void>(this->runCycle());
    if (this->isConnected_watchdogBeatOut_OutputPort(0)) {
        this->watchdogBeatOut_out(0, static_cast<U32>(OBC::WatchdogSource::ADCS_FDIR));
    }
}

void AdcsFdirController::publishState_(const OBC::AdcsFdirDecision& decision) {
    this->tlmWrite_ADCS_FDIR_FAULT_LATCHED(this->m_faultLatched);
    this->tlmWrite_ADCS_FDIR_ACTIVE_SOURCE(this->m_faultLatched ? this->m_latchedSource : decision.activeSource);
    this->tlmWrite_ADCS_FDIR_LAST_FAILURE_COUNT(decision.failureCount);
    this->tlmWrite_ADCS_FDIR_ESCALATION_COUNT(this->m_escalationCount);
    this->tlmWrite_ADCS_FDIR_RECOVERY_COUNT(this->m_recoveryCount);
}

void AdcsFdirController::saturatingIncrement_(U32& value) {
    if (value < std::numeric_limits<U32>::max()) {
        value++;
    }
}

void AdcsFdirController::submitFault_(const OBC::AdcsFdirDecision& decision) {
    if (this->m_recoverySink == nullptr) {
        return;
    }

    switch (decision.activeSource.e) {
        case OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT:
            this->m_recoverySink->submitAdcsPollTransportFault(decision.failureCount);
            break;
        case OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS:
            this->m_recoverySink->submitAdcsPollFreshnessFault(decision.failureCount);
            break;
        default:
            break;
    }
}

void AdcsFdirController::clearFault_() {
    if (this->m_recoverySink != nullptr) {
        switch (this->m_latchedSource.e) {
            case OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT:
                this->m_recoverySink->clearAdcsPollTransportFault(this->m_lastFailureCount);
                break;
            case OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS:
                this->m_recoverySink->clearAdcsPollFreshnessFault(this->m_lastFailureCount);
                break;
            default:
                break;
        }
    }

    this->m_faultLatched = false;
    this->m_latchedSource = OBC::RecoveryIncidentSource::NONE;
}

}  // namespace OBC
