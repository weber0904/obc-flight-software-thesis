#ifndef OBC_COMPONENTS_GROUNDLINKHEALTHPROVIDER_RUNTIME_HPP
#define OBC_COMPONENTS_GROUNDLINKHEALTHPROVIDER_RUNTIME_HPP

#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Components/GroundLinkDriver/GroundLinkObservationRuntime.hpp"
#include "OBC/Types/CommBandEnumAc.hpp"

namespace OBC {

enum class CommLinkAvailabilityReason : U32 {
    HEALTHY_ACTIVITY = 0U,
    CONNECTED_ONLY_FALLBACK = 1U,
    DISCONNECTED = 2U,
    STALE_ACTIVITY = 3U,
};

inline const char* commLinkAvailabilityReasonName(const OBC::CommLinkAvailabilityReason reason) {
    switch (reason) {
        case OBC::CommLinkAvailabilityReason::HEALTHY_ACTIVITY:
            return "HEALTHY_ACTIVITY";
        case OBC::CommLinkAvailabilityReason::CONNECTED_ONLY_FALLBACK:
            return "CONNECTED_ONLY_FALLBACK";
        case OBC::CommLinkAvailabilityReason::STALE_ACTIVITY:
            return "STALE_ACTIVITY";
        case OBC::CommLinkAvailabilityReason::DISCONNECTED:
        default:
            return "DISCONNECTED";
    }
}

struct CommLinkHealthView {
    OBC::CommBand band = OBC::CommBand::SBAND;
    OBC::COMM::GroundLinkBackendMode backendMode = OBC::COMM::GroundLinkBackendMode::DISABLED;
    bool connected = false;
    bool available = false;
    U32 activityAgeTicks = 0U;
    U32 rxAgeTicks = 0U;
    U32 txAgeTicks = 0U;
    bool errorGrowthThisCycle = false;
    OBC::CommLinkAvailabilityReason availabilityReason = OBC::CommLinkAvailabilityReason::DISCONNECTED;
};

}  // namespace OBC

#endif
