#ifndef OBC_COMPONENTS_EPSFDIRCONTROLLER_EPSFDIRPOLICY_HPP
#define OBC_COMPONENTS_EPSFDIRCONTROLLER_EPSFDIRPOLICY_HPP

#include "OBC/Components/EpsFdirController/EpsFdirRuntime.hpp"
#include "OBC/Types/SatModeEnumAc.hpp"

namespace OBC {

struct EpsFdirDecision {
    bool shouldEnterRetry = false;
    bool shouldLatchFault = false;
    bool shouldClearFault = false;
    bool shouldRequestSafe = false;
    bool faultLatched = false;
    U32 failureCount = 0U;
    OBC::SatMode currentMode = OBC::SatMode::SAFE;
    OBC::SatMode targetMode = OBC::SatMode::SAFE;
};

class EpsFdirPolicy final {
  public:
    static constexpr U32 ESCALATION_FAILURE_THRESHOLD = 3U;

    static OBC::EpsFdirDecision evaluate(const OBC::EPS::PollHealthState& health,
                                         bool currentlyLatched,
                                         OBC::SatMode currentMode);
};

}  // namespace OBC

#endif
