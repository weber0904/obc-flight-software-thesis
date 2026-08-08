#ifndef OBC_COMPONENTS_ADCSFDIRCONTROLLER_ADCSFDIRRUNTIME_HPP
#define OBC_COMPONENTS_ADCSFDIRCONTROLLER_ADCSFDIRRUNTIME_HPP

#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Types/RecoveryIncidentSourceEnumAc.hpp"

namespace OBC {

struct AdcsFdirDecision {
    bool shouldEnterRetry = false;
    bool shouldLatchFault = false;
    bool shouldClearFault = false;
    bool faultLatched = false;
    U32 failureCount = 0U;
    OBC::RecoveryIncidentSource activeSource = OBC::RecoveryIncidentSource::NONE;
};

}  // namespace OBC

#endif
