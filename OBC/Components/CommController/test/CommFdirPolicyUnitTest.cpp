#include "OBC/Components/CommController/CommFdirPolicy.hpp"

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

    OBC::CommFdirDecision decision =
        OBC::CommFdirPolicy::evaluate(false, false, 2U, 0U, 3U, false, OBC::CommFdirFaultKind::NONE);
    ok = check(!decision.shouldLatchFault, "unavailable below threshold should not latch") && ok;

    decision =
        OBC::CommFdirPolicy::evaluate(false, false, 3U, 0U, 3U, false, OBC::CommFdirFaultKind::NONE);
    ok = check(decision.shouldLatchFault, "unavailable at threshold should latch") && ok;
    ok = check(decision.kind == OBC::CommFdirFaultKind::PRIMARY_UNAVAILABLE,
               "unavailable threshold should map to PRIMARY_UNAVAILABLE") && ok;

    decision =
        OBC::CommFdirPolicy::evaluate(true, true, 0U, 3U, 3U, false, OBC::CommFdirFaultKind::NONE);
    ok = check(decision.shouldLatchFault, "transport growth at threshold should latch") && ok;
    ok = check(decision.kind == OBC::CommFdirFaultKind::PRIMARY_TRANSPORT,
               "transport threshold should map to PRIMARY_TRANSPORT") && ok;

    decision = OBC::CommFdirPolicy::evaluate(
        true, false, 0U, 0U, 3U, true, OBC::CommFdirFaultKind::PRIMARY_TRANSPORT);
    ok = check(decision.shouldClearFault, "healthy primary with no error growth should clear fault") && ok;

    decision = OBC::CommFdirPolicy::evaluate(
        false, false, 3U, 0U, 3U, true, OBC::CommFdirFaultKind::PRIMARY_TRANSPORT);
    ok = check(!decision.shouldLatchFault,
               "latched transport fault should not switch to unavailable without a healthy clear") && ok;
    ok = check(!decision.shouldClearFault,
               "continued unhealthy primary should not clear the latched transport fault") && ok;
    ok = check(decision.kind == OBC::CommFdirFaultKind::PRIMARY_TRANSPORT,
               "latched transport fault should remain authoritative until healthy clear") && ok;

    return ok ? 0 : 1;
}
