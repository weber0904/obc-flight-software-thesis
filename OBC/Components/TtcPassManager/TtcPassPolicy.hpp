#ifndef OBC_COMPONENTS_TTCPASSMANAGER_TTCPASSPOLICY_HPP
#define OBC_COMPONENTS_TTCPASSMANAGER_TTCPASSPOLICY_HPP

#include "OBC/Components/TtcPassManager/TtcPassRuntime.hpp"

namespace OBC {
namespace TtcPassPolicy {

constexpr U32 GPS_STALE_THRESHOLD_SEC = 15U;

bool isValidPassWindow(const OBC::TtcPassWindow& window);

bool isWindowActive(U64 nowUnixSec, const OBC::TtcPassWindow& window);

bool convertGpsUtcToUnixSec(U32 utcDateYmd, U32 utcSecondsOfDay, U64& unixSec);

void updateGpsFreshnessTracker(OBC::TtcGpsFreshnessTracker& tracker, U32 acceptedSentenceCount, U32 wallclockSec);

bool isGpsFresh(const OBC::TtcGpsFreshnessTracker& tracker, U32 wallclockSec, U32 staleThresholdSec);

U32 updateLossOfLockTimer(OBC::TtcCommLossTracker& tracker,
                          U32 wallclockSec,
                          bool currentModeIsTtc,
                          bool anyLinkAvailable);

}  // namespace TtcPassPolicy
}  // namespace OBC

#endif
