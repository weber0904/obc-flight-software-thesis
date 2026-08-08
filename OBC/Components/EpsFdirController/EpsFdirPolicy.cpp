#include "OBC/Components/EpsFdirController/EpsFdirPolicy.hpp"

namespace OBC {

namespace {

bool shouldRequestSafeForMode(OBC::SatMode mode) {
    return mode == OBC::SatMode::IDLE || mode == OBC::SatMode::PAYLOAD || mode == OBC::SatMode::TTC;
}

}  // namespace

OBC::EpsFdirDecision EpsFdirPolicy::evaluate(const OBC::EPS::PollHealthState& health,
                                             const bool currentlyLatched,
                                             const OBC::SatMode currentMode) {
    OBC::EpsFdirDecision decision = {};
    decision.failureCount = health.consecutivePollFailures;
    decision.currentMode = currentMode;
    decision.targetMode = currentMode;

    if (health.lastPollSucceeded) {
        decision.shouldClearFault = currentlyLatched;
        return decision;
    }

    if (health.consecutivePollFailures >= ESCALATION_FAILURE_THRESHOLD) {
        decision.faultLatched = true;
        if (!currentlyLatched) {
            decision.shouldLatchFault = true;
            if (shouldRequestSafeForMode(currentMode)) {
                decision.shouldRequestSafe = true;
                decision.targetMode = OBC::SatMode::SAFE;
            }
        }
        return decision;
    }

    if (health.consecutivePollFailures > 0U) {
        decision.shouldEnterRetry = true;
    }
    return decision;
}

}  // namespace OBC
