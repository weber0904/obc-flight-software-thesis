#ifndef OBC_COMPONENTS_GROUNDLINKDRIVER_GROUNDLINKOBSERVATIONRUNTIME_HPP
#define OBC_COMPONENTS_GROUNDLINKDRIVER_GROUNDLINKOBSERVATIONRUNTIME_HPP

#include <cstdint>

namespace OBC {
namespace COMM {

enum class GroundLinkBackendMode : std::uint8_t {
    DISABLED = 0U,
    DIRECT_TCP = 1U,
    COMM_CSP = 2U,
};

enum class GroundLinkHealthSemantics : std::uint8_t {
    DISABLED = 0U,
    ACTIVE_COMM_CSP = 1U,
    CONNECTED_ONLY_FALLBACK = 2U,
};

inline const char* groundLinkHealthSemanticsName(const GroundLinkHealthSemantics semantics) {
    switch (semantics) {
        case GroundLinkHealthSemantics::ACTIVE_COMM_CSP:
            return "ACTIVE_COMM_CSP";
        case GroundLinkHealthSemantics::CONNECTED_ONLY_FALLBACK:
            return "CONNECTED_ONLY_FALLBACK";
        case GroundLinkHealthSemantics::DISABLED:
        default:
            return "DISABLED";
    }
}

struct GroundLinkObservationState {
    GroundLinkBackendMode mode = GroundLinkBackendMode::DISABLED;
    GroundLinkHealthSemantics healthSemantics = GroundLinkHealthSemantics::DISABLED;
    bool connected = false;
    std::uint32_t txChunks = 0U;
    std::uint32_t rxChunks = 0U;
    std::uint32_t txBytes = 0U;
    std::uint32_t rxBytes = 0U;
    std::uint32_t txErrors = 0U;
    std::uint32_t rxErrors = 0U;
    std::uint32_t successfulStatusObservations = 0U;
};

}  // namespace COMM
}  // namespace OBC

#endif
