#include "OBC/Components/AdcsFdirController/AdcsFdirPolicy.hpp"

namespace OBC {

OBC::AdcsFdirDecision AdcsFdirPolicy::evaluate(const OBC::ADCS::PollHealthState& health,
                                               bool currentlyLatched,
                                               OBC::RecoveryIncidentSource latchedSource) {
    OBC::AdcsFdirDecision decision = {};
    decision.faultLatched = currentlyLatched;
    decision.activeSource = OBC::RecoveryIncidentSource::NONE;
    if (currentlyLatched) {
        decision.activeSource = latchedSource;
    }

    if (health.lastScheduledValidRefresh) {
        decision.shouldClearFault = currentlyLatched;
        if (currentlyLatched) {
            decision.activeSource = latchedSource;
        }
        return decision;
    }

    // Keep the latched ADCS source authoritative until a healthy scheduled poll clears it.
    // This prevents the controller from opening a second shared incident for an alternate
    // failure mode before the first one closes.
    if (currentlyLatched) {
        return decision;
    }

    if (health.consecutiveTransportFailures >= ESCALATION_FAILURE_THRESHOLD) {
        decision.shouldLatchFault = true;
        decision.failureCount = health.consecutiveTransportFailures;
        decision.activeSource = OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT;
        return decision;
    }

    if (health.consecutiveNoValidRefresh >= ESCALATION_FAILURE_THRESHOLD) {
        decision.shouldLatchFault = true;
        decision.failureCount = health.consecutiveNoValidRefresh;
        decision.activeSource = OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS;
        return decision;
    }

    if (health.consecutiveTransportFailures > 0U) {
        decision.shouldEnterRetry = true;
        decision.failureCount = health.consecutiveTransportFailures;
        decision.activeSource = OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT;
        return decision;
    }

    if (health.consecutiveNoValidRefresh > 0U) {
        decision.shouldEnterRetry = true;
        decision.failureCount = health.consecutiveNoValidRefresh;
        decision.activeSource = OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS;
    }

    return decision;
}

}  // namespace OBC
