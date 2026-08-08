#ifndef OBC_COMPONENTS_ADCSBRIDGE_ADCSRUNTIME_HPP
#define OBC_COMPONENTS_ADCSBRIDGE_ADCSRUNTIME_HPP

#include "Fw/Types/BasicTypes.hpp"

namespace OBC {

namespace ADCS {

struct PollHealthState {
    bool lastScheduledTransportOk = false;
    bool lastScheduledValidRefresh = false;
    bool hasValidState = false;
    U32 consecutiveTransportFailures = 0U;
    U32 consecutiveNoValidRefresh = 0U;
    U32 cumulativeTransportErrors = 0U;
    U32 cumulativeNoValidRefresh = 0U;
};

}  // namespace ADCS

class IAdcsFdirHealthProvider {
  public:
    virtual ~IAdcsFdirHealthProvider() = default;
    virtual bool getPollHealthForRuntime(OBC::ADCS::PollHealthState& state) const = 0;
};

}  // namespace OBC

#endif
