#include "OBC/Components/CommController/CommFdirPolicy.hpp"

namespace OBC {

OBC::CommFdirDecision CommFdirPolicy::evaluate(bool primaryAvailable,
                                               bool primaryErrorGrowth,
                                               U32 consecutivePrimaryUnavailable,
                                               U32 consecutivePrimaryTransportGrowth,
                                               U32 unavailableFailureThreshold,
                                               bool currentlyLatched,
                                               OBC::CommFdirFaultKind latchedKind) {
    OBC::CommFdirDecision decision = {};

    if (primaryAvailable && !primaryErrorGrowth) {
        decision.shouldClearFault = currentlyLatched;
        decision.kind = currentlyLatched ? latchedKind : OBC::CommFdirFaultKind::NONE;
        return decision;
    }

    // Keep the currently latched COMM fault authoritative until the primary link
    // has one healthy scheduled cycle and clears. This avoids opening a second
    // shared COMM incident when the observed degradation mode changes mid-fault.
    if (currentlyLatched) {
        decision.kind = latchedKind;
        return decision;
    }

    if (consecutivePrimaryUnavailable >= unavailableFailureThreshold) {
        decision.shouldLatchFault = true;
        decision.kind = OBC::CommFdirFaultKind::PRIMARY_UNAVAILABLE;
        decision.failureCount = consecutivePrimaryUnavailable;
        return decision;
    }

    if (consecutivePrimaryTransportGrowth >= unavailableFailureThreshold) {
        decision.shouldLatchFault = true;
        decision.kind = OBC::CommFdirFaultKind::PRIMARY_TRANSPORT;
        decision.failureCount = consecutivePrimaryTransportGrowth;
    }

    return decision;
}

}  // namespace OBC
