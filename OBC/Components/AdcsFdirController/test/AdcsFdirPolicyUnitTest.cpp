#include "OBC/Components/AdcsFdirController/AdcsFdirPolicy.hpp"

#include <iostream>
#include <string>

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

}  // namespace

int main() {
    bool ok = true;

    OBC::ADCS::PollHealthState health = {};
    health.consecutiveTransportFailures = 2U;
    OBC::AdcsFdirDecision decision =
        OBC::AdcsFdirPolicy::evaluate(health, false, OBC::RecoveryIncidentSource::NONE);
    ok = check(decision.shouldEnterRetry, "transport failures below threshold should stay retry-only") && ok;
    ok = check(!decision.shouldLatchFault, "transport retries below threshold must not latch") && ok;

    health = {};
    health.consecutiveTransportFailures = 3U;
    decision = OBC::AdcsFdirPolicy::evaluate(health, false, OBC::RecoveryIncidentSource::NONE);
    ok = check(decision.shouldLatchFault, "transport failures at threshold should latch") && ok;
    ok = check(decision.activeSource == OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT,
               "transport threshold should map to ADCS_POLL_TRANSPORT") && ok;

    health = {};
    health.consecutiveNoValidRefresh = 3U;
    decision = OBC::AdcsFdirPolicy::evaluate(health, false, OBC::RecoveryIncidentSource::NONE);
    ok = check(decision.shouldLatchFault, "freshness failures at threshold should latch") && ok;
    ok = check(decision.activeSource == OBC::RecoveryIncidentSource::ADCS_POLL_FRESHNESS,
               "freshness threshold should map to ADCS_POLL_FRESHNESS") && ok;

    health = {};
    health.lastScheduledValidRefresh = true;
    decision = OBC::AdcsFdirPolicy::evaluate(
        health, true, OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
    ok = check(decision.shouldClearFault, "healthy scheduled refresh should clear latched fault") && ok;

    health = {};
    health.consecutiveNoValidRefresh = 3U;
    decision = OBC::AdcsFdirPolicy::evaluate(
        health, true, OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT);
    ok = check(!decision.shouldLatchFault, "latched transport fault should not switch to freshness without clear") && ok;
    ok = check(!decision.shouldClearFault, "unhealthy transition should not clear latched transport fault") && ok;
    ok = check(decision.activeSource == OBC::RecoveryIncidentSource::ADCS_POLL_TRANSPORT,
               "latched transport fault should remain the active source until healthy clear") && ok;

    return ok ? 0 : 1;
}
