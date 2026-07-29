#include "OBC/Components/EpsFdirController/EpsFdirPolicy.hpp"

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

OBC::EPS::PollHealthState makeHealth(bool success, U32 failures) {
    OBC::EPS::PollHealthState state = {};
    state.lastPollSucceeded = success;
    state.consecutivePollFailures = failures;
    state.cumulativePollErrors = failures;
    return state;
}

}  // namespace

int main() {
    bool ok = true;

    OBC::EpsFdirDecision decision =
        OBC::EpsFdirPolicy::evaluate(makeHealth(false, 1U), false, OBC::SatMode::IDLE);
    ok = check(decision.shouldEnterRetry, "failure count 1 should be retry-only") && ok;
    ok = check(!decision.shouldLatchFault, "failure count 1 should not latch fault") && ok;

    decision = OBC::EpsFdirPolicy::evaluate(makeHealth(false, 2U), false, OBC::SatMode::PAYLOAD);
    ok = check(decision.shouldEnterRetry, "failure count 2 should remain retry-only") && ok;
    ok = check(!decision.shouldRequestSafe, "failure count 2 should not request SAFE") && ok;

    decision = OBC::EpsFdirPolicy::evaluate(makeHealth(false, 3U), false, OBC::SatMode::PAYLOAD);
    ok = check(decision.shouldLatchFault, "failure count 3 should latch fault") && ok;
    ok = check(decision.shouldRequestSafe, "active mode fault should request SAFE") && ok;
    ok = check(decision.targetMode == OBC::SatMode::SAFE, "active mode fault target should be SAFE") && ok;

    decision = OBC::EpsFdirPolicy::evaluate(makeHealth(false, 5U), true, OBC::SatMode::IDLE);
    ok = check(!decision.shouldLatchFault, "latched fault should not relatch") && ok;
    ok = check(!decision.shouldRequestSafe, "latched fault should not re-request SAFE") && ok;

    decision = OBC::EpsFdirPolicy::evaluate(makeHealth(false, 3U), false, OBC::SatMode::SAFE);
    ok = check(decision.shouldLatchFault, "SAFE mode should still latch fault") && ok;
    ok = check(!decision.shouldRequestSafe, "SAFE mode should not request duplicate SAFE") && ok;

    decision = OBC::EpsFdirPolicy::evaluate(makeHealth(true, 0U), true, OBC::SatMode::SAFE);
    ok = check(decision.shouldClearFault, "first success after fault should clear latch") && ok;
    ok = check(!decision.faultLatched, "cleared fault should not remain latched") && ok;

    return ok ? 0 : 1;
}
