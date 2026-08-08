#include "OBC/Components/TtcPassManager/TtcPassPolicy.hpp"

#include <iostream>

namespace {

bool check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

}  // namespace

int main() {
    bool ok = true;

    const OBC::TtcPassWindow validWindow{100U, 200U};
    ok = check(OBC::TtcPassPolicy::isValidPassWindow(validWindow), "Expected valid pass window") && ok;
    ok = check(OBC::TtcPassPolicy::isWindowActive(100U, validWindow), "Expected inclusive window start") && ok;
    ok = check(OBC::TtcPassPolicy::isWindowActive(199U, validWindow), "Expected active window before end") && ok;
    ok = check(!OBC::TtcPassPolicy::isWindowActive(200U, validWindow), "Expected exclusive window end") && ok;
    ok = check(!OBC::TtcPassPolicy::isValidPassWindow(OBC::TtcPassWindow{0U, 200U}),
               "Expected zero start rejected") &&
         ok;
    ok = check(!OBC::TtcPassPolicy::isValidPassWindow(OBC::TtcPassWindow{200U, 200U}),
               "Expected equal start and end rejected") &&
         ok;

    U64 unixSec = 0U;
    ok = check(OBC::TtcPassPolicy::convertGpsUtcToUnixSec(20260513U, 0U, unixSec),
               "Expected midnight UTC conversion to succeed") &&
         ok;
    ok = check(OBC::TtcPassPolicy::convertGpsUtcToUnixSec(20260513U, 3600U, unixSec),
               "Expected UTC conversion to succeed") &&
         ok;
    ok = check(unixSec > 0U, "Expected converted Unix time to be positive") && ok;
    ok = check(!OBC::TtcPassPolicy::convertGpsUtcToUnixSec(20260230U, 1U, unixSec),
               "Expected impossible UTC date rejected") &&
         ok;
    ok = check(!OBC::TtcPassPolicy::convertGpsUtcToUnixSec(20260513U, 86400U, unixSec),
               "Expected out-of-range second-of-day rejected") &&
         ok;

    OBC::TtcGpsFreshnessTracker tracker = {};
    ok = check(!OBC::TtcPassPolicy::isGpsFresh(tracker, 5U, OBC::TtcPassPolicy::GPS_STALE_THRESHOLD_SEC),
               "Expected empty GPS freshness tracker to be stale") &&
         ok;
    OBC::TtcPassPolicy::updateGpsFreshnessTracker(tracker, 3U, 5U);
    ok = check(OBC::TtcPassPolicy::isGpsFresh(tracker, 20U, OBC::TtcPassPolicy::GPS_STALE_THRESHOLD_SEC),
               "Expected tracker to remain fresh within threshold") &&
         ok;
    ok = check(!OBC::TtcPassPolicy::isGpsFresh(tracker, 21U, OBC::TtcPassPolicy::GPS_STALE_THRESHOLD_SEC),
               "Expected tracker to become stale past threshold") &&
         ok;
    OBC::TtcPassPolicy::updateGpsFreshnessTracker(tracker, 4U, 21U);
    ok = check(OBC::TtcPassPolicy::isGpsFresh(tracker, 21U, OBC::TtcPassPolicy::GPS_STALE_THRESHOLD_SEC),
               "Expected accepted sentence advance to refresh tracker") &&
         ok;

    OBC::TtcCommLossTracker commLoss = {};
    ok = check(OBC::TtcPassPolicy::updateLossOfLockTimer(commLoss, 10U, false, false) == 0U,
               "Expected non-TTC mode to clear COMM loss timer") &&
         ok;
    ok = check(OBC::TtcPassPolicy::updateLossOfLockTimer(commLoss, 10U, true, true) == 0U,
               "Expected available COMM link to clear loss timer") &&
         ok;
    ok = check(OBC::TtcPassPolicy::updateLossOfLockTimer(commLoss, 10U, true, false) == 0U,
               "Expected TTC COMM loss timer to start at zero elapsed seconds") &&
         ok;
    ok = check(OBC::TtcPassPolicy::updateLossOfLockTimer(commLoss, 10U, true, false) == 0U,
               "Expected repeated checks inside the same second to preserve timer value") &&
         ok;
    ok = check(OBC::TtcPassPolicy::updateLossOfLockTimer(commLoss, 14U, true, false) == 4U,
               "Expected TTC COMM loss timer to advance by elapsed seconds") &&
         ok;
    ok = check(OBC::TtcPassPolicy::updateLossOfLockTimer(commLoss, 15U, true, true) == 0U,
               "Expected COMM recovery to clear the timer tracker") &&
         ok;

    return ok ? 0 : 1;
}
