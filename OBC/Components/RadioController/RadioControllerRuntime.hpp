#ifndef OBC_COMPONENTS_RADIOCONTROLLER_RUNTIME_HPP
#define OBC_COMPONENTS_RADIOCONTROLLER_RUNTIME_HPP

#include "Fw/Types/BasicTypes.hpp"
#include "simulators/comm/RadioTransport.hpp"

namespace OBC {

enum class RadioObservationResult : U32 {
    NO_SAMPLE = 0U,
    OK = 1U,
    TIMEOUT = 2U,
    TRANSPORT_ERROR = 3U,
    INVALID_RESPONSE = 4U,
    UNSUPPORTED = 5U,
};

inline const char* radioObservationResultName(const OBC::RadioObservationResult result) {
    switch (result) {
        case OBC::RadioObservationResult::OK:
            return "OK";
        case OBC::RadioObservationResult::TIMEOUT:
            return "TIMEOUT";
        case OBC::RadioObservationResult::TRANSPORT_ERROR:
            return "TRANSPORT_ERROR";
        case OBC::RadioObservationResult::INVALID_RESPONSE:
            return "INVALID_RESPONSE";
        case OBC::RadioObservationResult::UNSUPPORTED:
            return "UNSUPPORTED";
        case OBC::RadioObservationResult::NO_SAMPLE:
        default:
            return "NO_SAMPLE";
    }
}

struct RadioObservationState {
    bool haveSample = false;
    U32 statusAgeTicks = 0U;
    OBC::RadioObservationResult lastResult = OBC::RadioObservationResult::NO_SAMPLE;
    OBC::COMM::RadioStatus lastStatus = {};
};

}  // namespace OBC

#endif
