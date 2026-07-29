#include "OBC/Components/EpsFdirController/EpsFdirController.hpp"
#include "OBC/Types/WatchdogSourceEnumAc.hpp"

#include <limits>

namespace OBC {

EpsFdirController::EpsFdirController(const char* const compName)
    : EpsFdirControllerComponentBase(compName),
      m_modeControl(nullptr),
      m_healthProvider(nullptr),
      m_recoverySink(nullptr),
      m_faultLatched(false),
      m_lastRequestedSafe(false),
      m_lastFailureCount(0U),
      m_escalationCount(0U),
      m_recoveryCount(0U) {}

EpsFdirController::~EpsFdirController() = default;

void EpsFdirController::configureRuntime(OBC::IModeSafetyModeControl* modeControl,
                                         const OBC::IEpsFdirHealthProvider* healthProvider,
                                         OBC::IRecoveryRequestSink* recoverySink) {
    this->m_modeControl = modeControl;
    this->m_healthProvider = healthProvider;
    this->m_recoverySink = recoverySink;
}

#ifdef BUILD_UT
void EpsFdirController::setCountersForTest(U32 escalationCount, U32 recoveryCount) {
    this->m_escalationCount = escalationCount;
    this->m_recoveryCount = recoveryCount;
}
#endif

OBC::EpsFdirDecision EpsFdirController::runCycle() {
    if (this->m_modeControl == nullptr || this->m_healthProvider == nullptr) {
        return {};
    }

    OBC::EPS::PollHealthState health = {};
    if (!this->m_healthProvider->getPollHealthForRuntime(health)) {
        return {};
    }

    const OBC::EpsFdirDecision decision =
        OBC::EpsFdirPolicy::evaluate(health, this->m_faultLatched, this->m_modeControl->getModeForRuntime());

    if (decision.shouldEnterRetry && decision.failureCount != this->m_lastFailureCount) {
        this->log_WARNING_LO_EPS_FDIR_RETRYING(decision.failureCount);
    }

    if (decision.shouldLatchFault) {
        this->m_faultLatched = true;
        this->m_lastRequestedSafe = decision.shouldRequestSafe;
        saturatingIncrement_(this->m_escalationCount);
        this->log_WARNING_HI_EPS_FDIR_FAULT_ENTERED(decision.failureCount, decision.currentMode, decision.shouldRequestSafe);
        if (this->m_recoverySink != nullptr) {
            this->m_recoverySink->submitEpsTimeoutFault(decision.failureCount);
        }
    } else if (decision.shouldClearFault) {
        this->m_faultLatched = false;
        this->m_lastRequestedSafe = false;
        saturatingIncrement_(this->m_recoveryCount);
        this->log_ACTIVITY_HI_EPS_FDIR_FAULT_CLEARED(this->m_lastFailureCount);
        if (this->m_recoverySink != nullptr) {
            this->m_recoverySink->clearEpsTimeoutFault(this->m_lastFailureCount);
        }
    }

    this->m_lastFailureCount = decision.failureCount;
    this->publishState_(decision);
    return decision;
}

void EpsFdirController::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    static_cast<void>(this->runCycle());
    if (this->isConnected_watchdogBeatOut_OutputPort(0)) {
        this->watchdogBeatOut_out(0, static_cast<U32>(OBC::WatchdogSource::EPS_FDIR));
    }
}

void EpsFdirController::publishState_(const OBC::EpsFdirDecision& decision) {
    this->tlmWrite_EPS_FDIR_FAULT_LATCHED(this->m_faultLatched);
    this->tlmWrite_EPS_FDIR_LAST_FAILURE_COUNT(decision.failureCount);
    this->tlmWrite_EPS_FDIR_ESCALATION_COUNT(this->m_escalationCount);
    this->tlmWrite_EPS_FDIR_RECOVERY_COUNT(this->m_recoveryCount);
    this->tlmWrite_EPS_FDIR_LAST_REQUESTED_SAFE(this->m_lastRequestedSafe);
}

void EpsFdirController::saturatingIncrement_(U32& value) {
    if (value < std::numeric_limits<U32>::max()) {
        value++;
    }
}

}  // namespace OBC
