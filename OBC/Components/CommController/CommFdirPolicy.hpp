#ifndef OBC_COMPONENTS_COMMCONTROLLER_COMMFDIRPOLICY_HPP
#define OBC_COMPONENTS_COMMCONTROLLER_COMMFDIRPOLICY_HPP

#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Components/CommController/CommControllerRuntime.hpp"

namespace OBC {

struct CommFdirDecision {
    bool shouldLatchFault = false;
    bool shouldClearFault = false;
    OBC::CommFdirFaultKind kind = OBC::CommFdirFaultKind::NONE;
    U32 failureCount = 0U;
};

class CommFdirPolicy final {
  public:
    static OBC::CommFdirDecision evaluate(bool primaryAvailable,
                                          bool primaryErrorGrowth,
                                          U32 consecutivePrimaryUnavailable,
                                          U32 consecutivePrimaryTransportGrowth,
                                          U32 unavailableFailureThreshold,
                                          bool currentlyLatched,
                                          OBC::CommFdirFaultKind latchedKind);
};

}  // namespace OBC

#endif
