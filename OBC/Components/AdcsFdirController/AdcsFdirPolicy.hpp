#ifndef OBC_COMPONENTS_ADCSFDIRCONTROLLER_ADCSFDIRPOLICY_HPP
#define OBC_COMPONENTS_ADCSFDIRCONTROLLER_ADCSFDIRPOLICY_HPP

#include "OBC/Components/AdcsBridge/AdcsRuntime.hpp"
#include "OBC/Components/AdcsFdirController/AdcsFdirRuntime.hpp"

namespace OBC {

class AdcsFdirPolicy final {
  public:
    static constexpr U32 ESCALATION_FAILURE_THRESHOLD = 3U;

    static OBC::AdcsFdirDecision evaluate(const OBC::ADCS::PollHealthState& health,
                                          bool currentlyLatched,
                                          OBC::RecoveryIncidentSource latchedSource);
};

}  // namespace OBC

#endif
