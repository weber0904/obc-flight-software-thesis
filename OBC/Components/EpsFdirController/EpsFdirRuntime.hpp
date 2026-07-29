#ifndef OBC_COMPONENTS_EPSFDIRCONTROLLER_EPSFDIRRUNTIME_HPP
#define OBC_COMPONENTS_EPSFDIRCONTROLLER_EPSFDIRRUNTIME_HPP

#include "Fw/Types/BasicTypes.hpp"

namespace OBC {
namespace EPS {

struct PollHealthState {
    bool cacheValid = false;
    bool lastPollSucceeded = true;
    U32 consecutivePollFailures = 0U;
    U32 cumulativePollErrors = 0U;
};

}  // namespace EPS

class IEpsFdirHealthProvider {
  public:
    virtual ~IEpsFdirHealthProvider() = default;

    virtual bool getPollHealthForRuntime(OBC::EPS::PollHealthState& state) const = 0;
};

}  // namespace OBC

#endif
