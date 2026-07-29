#include "OBC/Components/ModeSafetyController/ModeSafetyPolicy.hpp"

namespace OBC {

bool ModeSafetyDecision::shouldTransition() const {
    return this->action != ModeSafetyAction::NONE && this->targetMode != this->currentMode;
}

ModeSafetyDecision ModeSafetyPolicy::evaluate(OBC::SatMode currentMode, bool hasValidEpsStatus, F32 soc) {
    ModeSafetyDecision decision = {};
    decision.hasValidEpsStatus = hasValidEpsStatus;
    decision.currentMode = currentMode;
    decision.targetMode = currentMode;
    decision.soc = soc;
    decision.action = ModeSafetyAction::NONE;

    if (!hasValidEpsStatus) {
        return decision;
    }

    if (currentMode == OBC::SatMode::SAFE && soc < SAFE_TO_HELL_SOC) {
        decision.targetMode = OBC::SatMode::HELL;
        decision.action = ModeSafetyAction::SAFE_TO_HELL;
        return decision;
    }

    if (currentMode == OBC::SatMode::HELL && soc > HELL_TO_SAFE_SOC) {
        decision.targetMode = OBC::SatMode::SAFE;
        decision.action = ModeSafetyAction::HELL_TO_SAFE;
        return decision;
    }

    if (currentMode == OBC::SatMode::PAYLOAD) {
        if (soc < ACTIVE_TO_SAFE_SOC) {
            decision.targetMode = OBC::SatMode::SAFE;
            decision.action = ModeSafetyAction::ACTIVE_TO_SAFE;
            return decision;
        }

        if (soc < PAYLOAD_TO_IDLE_SOC) {
            decision.targetMode = OBC::SatMode::IDLE;
            decision.action = ModeSafetyAction::PAYLOAD_TO_IDLE;
            return decision;
        }
    }

    if (isModeWithSafeFallbackFloor_(currentMode) && soc < ACTIVE_TO_SAFE_SOC) {
        decision.targetMode = OBC::SatMode::SAFE;
        decision.action = ModeSafetyAction::ACTIVE_TO_SAFE;
        return decision;
    }

    return decision;
}

bool ModeSafetyPolicy::isModeWithSafeFallbackFloor_(OBC::SatMode mode) {
    return mode == OBC::SatMode::IDLE || mode == OBC::SatMode::TTC;
}

}  // namespace OBC
