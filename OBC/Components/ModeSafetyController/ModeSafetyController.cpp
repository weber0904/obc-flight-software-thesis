#include "OBC/Components/ModeSafetyController/ModeSafetyController.hpp"
#include "OBC/Types/WatchdogSourceEnumAc.hpp"

namespace OBC {

ModeSafetyController::ModeSafetyController(const char* const compName)
    : ModeSafetyControllerComponentBase(compName),
      m_modeControl(nullptr),
      m_epsStatus(nullptr),
      m_transitionCount(0U),
      m_epsUnavailableLatched(false) {}

ModeSafetyController::~ModeSafetyController() = default;

void ModeSafetyController::configureRuntime(OBC::IModeSafetyModeControl* modeControl,
                                            const OBC::IModeSafetyEpsStatus* epsStatus) {
    this->m_modeControl = modeControl;
    this->m_epsStatus = epsStatus;
}

OBC::OperatorModeTransitionDecision ModeSafetyController::evaluateOperatorTransition(
    OBC::SatMode currentMode,
    OBC::SatMode requestedMode) const {
    OBC::OperatorModeTransitionDecision decision = {};

    const auto guardSoc = [this, &decision](F32 threshold) -> bool {
        OBC::EPS::StatusData status = {};
        if (this->m_epsStatus == nullptr || !this->m_epsStatus->getCachedStatusForRuntime(status)) {
            decision.reason = OBC::ModeTransitionRejectionReason::SOC_GUARD_UNAVAILABLE;
            return false;
        }

        if (status.soc > threshold) {
            return true;
        }

        decision.reason = OBC::ModeTransitionRejectionReason::SOC_GUARD_NOT_MET;
        return false;
    };

    if (currentMode == requestedMode) {
        decision.accepted = true;
        return decision;
    }

    if (requestedMode == OBC::SatMode::HELL) {
        decision.reason = OBC::ModeTransitionRejectionReason::INTERNAL_ONLY_TARGET;
        return decision;
    }

    if (currentMode == OBC::SatMode::SAFE) {
        if (requestedMode == OBC::SatMode::IDLE) {
            decision.accepted = guardSoc(OBC::ModeSafetyPolicy::SAFE_TO_IDLE_SOC);
        }
        return decision;
    }

    if (currentMode == OBC::SatMode::IDLE) {
        if (requestedMode == OBC::SatMode::SAFE || requestedMode == OBC::SatMode::TTC) {
            decision.accepted = true;
            return decision;
        }

        if (requestedMode == OBC::SatMode::PAYLOAD) {
            decision.accepted = guardSoc(OBC::ModeSafetyPolicy::IDLE_TO_PAYLOAD_SOC);
        }
        return decision;
    }

    if (currentMode == OBC::SatMode::PAYLOAD) {
        decision.accepted = requestedMode == OBC::SatMode::SAFE ||
                            requestedMode == OBC::SatMode::IDLE;
        return decision;
    }

    if (currentMode == OBC::SatMode::TTC) {
        decision.accepted = requestedMode == OBC::SatMode::SAFE ||
                            requestedMode == OBC::SatMode::IDLE;
        return decision;
    }

    if (currentMode == OBC::SatMode::HELL && requestedMode == OBC::SatMode::SAFE) {
        decision.accepted = guardSoc(OBC::ModeSafetyPolicy::HELL_TO_SAFE_SOC);
        return decision;
    }

    decision.reason = OBC::ModeTransitionRejectionReason::DISALLOWED_OPERATOR_TRANSITION;
    return decision;
}

OBC::ModeSafetyDecision ModeSafetyController::runCycle() {
    if (this->m_modeControl == nullptr || this->m_epsStatus == nullptr) {
        return {};
    }

    const OBC::SatMode currentMode = this->m_modeControl->getModeForRuntime();

    OBC::EPS::StatusData status = {};
    const bool hasValidEpsStatus =
        this->m_epsStatus->getCachedStatusForRuntime(status);
    const F32 soc = hasValidEpsStatus ? status.soc : 0.0F;

    OBC::ModeSafetyDecision decision = OBC::ModeSafetyPolicy::evaluate(currentMode, hasValidEpsStatus, soc);

    if (!hasValidEpsStatus) {
        if (!this->m_epsUnavailableLatched) {
            this->log_WARNING_HI_MODE_SAFETY_EPS_UNAVAILABLE();
            this->m_epsUnavailableLatched = true;
        }
        this->publishDecision_(decision);
        return decision;
    }
    this->m_epsUnavailableLatched = false;

    if (decision.shouldTransition()) {
        OBC::ModeApplySource source = OBC::ModeApplySource::SafetyFallback;
        if (decision.action == OBC::ModeSafetyAction::HELL_TO_SAFE) {
            source = OBC::ModeApplySource::SafetyRecovery;
        } else if (decision.action == OBC::ModeSafetyAction::PAYLOAD_TO_IDLE) {
            source = OBC::ModeApplySource::SafetyPayloadExit;
        }
        this->m_modeControl->applyModeForInternalSource(decision.targetMode, source);
        this->m_transitionCount++;
        this->log_ACTIVITY_HI_MODE_SAFETY_TRANSITION(decision.currentMode, decision.targetMode, decision.soc);
    }

    this->publishDecision_(decision);
    return decision;
}

void ModeSafetyController::schedIn_handler(FwIndexType portNum, U32 context) {
    static_cast<void>(portNum);
    static_cast<void>(context);
    static_cast<void>(this->runCycle());
    if (this->isConnected_watchdogBeatOut_OutputPort(0)) {
        this->watchdogBeatOut_out(0, static_cast<U32>(OBC::WatchdogSource::MODE_SAFETY));
    }
}

void ModeSafetyController::publishDecision_(const OBC::ModeSafetyDecision& decision) {
    this->tlmWrite_MODE_SAFETY_EPS_VALID(decision.hasValidEpsStatus);
    this->tlmWrite_MODE_SAFETY_LAST_SOC(decision.soc);
    this->tlmWrite_MODE_SAFETY_LAST_CURRENT_MODE(decision.currentMode);
    this->tlmWrite_MODE_SAFETY_LAST_TARGET_MODE(decision.targetMode);
    this->tlmWrite_MODE_SAFETY_TRANSITION_COUNT(this->m_transitionCount);
}

}  // namespace OBC
