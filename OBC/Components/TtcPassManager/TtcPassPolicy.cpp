#include "OBC/Components/TtcPassManager/TtcPassPolicy.hpp"

#include <limits>

namespace OBC {
namespace TtcPassPolicy {

namespace {

bool isLeapYear(const U32 year) {
    return ((year % 4U) == 0U && (year % 100U) != 0U) || ((year % 400U) == 0U);
}

U32 daysInMonth(const U32 year, const U32 month) {
    switch (month) {
        case 1U:
        case 3U:
        case 5U:
        case 7U:
        case 8U:
        case 10U:
        case 12U:
            return 31U;
        case 4U:
        case 6U:
        case 9U:
        case 11U:
            return 30U;
        case 2U:
            return isLeapYear(year) ? 29U : 28U;
        default:
            return 0U;
    }
}

I64 daysFromCivil(I64 year, const U32 month, const U32 day) {
    year -= month <= 2U ? 1 : 0;
    const I64 era = (year >= 0 ? year : year - 399) / 400;
    const U32 yoe = static_cast<U32>(year - era * 400);
    const U32 monthIndex = month > 2U ? month - 3U : month + 9U;
    const U32 doy = (153U * monthIndex + 2U) / 5U + day - 1U;
    const U32 doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
    return era * 146097 + static_cast<I64>(doe) - 719468;
}

}  // namespace

bool isValidPassWindow(const OBC::TtcPassWindow& window) {
    return window.startUnixSec != 0U && window.endUnixSec != 0U && window.endUnixSec > window.startUnixSec;
}

bool isWindowActive(const U64 nowUnixSec, const OBC::TtcPassWindow& window) {
    return isValidPassWindow(window) && nowUnixSec >= window.startUnixSec && nowUnixSec < window.endUnixSec;
}

bool convertGpsUtcToUnixSec(const U32 utcDateYmd, const U32 utcSecondsOfDay, U64& unixSec) {
    unixSec = 0U;

    if (utcDateYmd == 0U || utcSecondsOfDay >= 86400U) {
        return false;
    }

    const U32 year = utcDateYmd / 10000U;
    const U32 month = (utcDateYmd / 100U) % 100U;
    const U32 day = utcDateYmd % 100U;
    if (year < 1970U || month < 1U || month > 12U) {
        return false;
    }

    const U32 maxDay = daysInMonth(year, month);
    if (day < 1U || day > maxDay) {
        return false;
    }

    const I64 daysSinceEpoch = daysFromCivil(static_cast<I64>(year), month, day);
    if (daysSinceEpoch < 0) {
        return false;
    }

    const U64 daySeconds = static_cast<U64>(daysSinceEpoch) * 86400ULL;
    const U64 result = daySeconds + static_cast<U64>(utcSecondsOfDay);
    if (result < daySeconds) {
        return false;
    }

    unixSec = result;
    return true;
}

void updateGpsFreshnessTracker(OBC::TtcGpsFreshnessTracker& tracker, const U32 acceptedSentenceCount, const U32 wallclockSec) {
    if (!tracker.haveAcceptedSentenceTimestamp || acceptedSentenceCount != tracker.lastAcceptedSentenceCount) {
        tracker.haveAcceptedSentenceTimestamp = true;
        tracker.lastAcceptedSentenceCount = acceptedSentenceCount;
        tracker.lastAcceptedSentenceWallclockSec = wallclockSec;
    }
}

bool isGpsFresh(const OBC::TtcGpsFreshnessTracker& tracker, const U32 wallclockSec, const U32 staleThresholdSec) {
    if (!tracker.haveAcceptedSentenceTimestamp) {
        return false;
    }

    if (wallclockSec < tracker.lastAcceptedSentenceWallclockSec) {
        return false;
    }

    return (wallclockSec - tracker.lastAcceptedSentenceWallclockSec) <= staleThresholdSec;
}

U32 updateLossOfLockTimer(OBC::TtcCommLossTracker& tracker,
                          const U32 wallclockSec,
                          const bool currentModeIsTtc,
                          const bool anyLinkAvailable) {
    if (!currentModeIsTtc || anyLinkAvailable) {
        tracker.active = false;
        tracker.noLinkStartWallclockSec = 0U;
        return 0U;
    }

    if (!tracker.active || wallclockSec < tracker.noLinkStartWallclockSec) {
        tracker.active = true;
        tracker.noLinkStartWallclockSec = wallclockSec;
        return 0U;
    }

    const U32 elapsedSec = wallclockSec - tracker.noLinkStartWallclockSec;
    if (elapsedSec >= std::numeric_limits<U32>::max()) {
        return std::numeric_limits<U32>::max();
    }
    return elapsedSec;
}

}  // namespace TtcPassPolicy
}  // namespace OBC
