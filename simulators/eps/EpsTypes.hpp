#ifndef OBC_SIMULATORS_EPS_EPSTYPES_HPP
#define OBC_SIMULATORS_EPS_EPSTYPES_HPP

#include <cstdint>

namespace OBC {
namespace EPS {

enum class ResultCode : std::uint8_t {
    OK = 0U,
    INVALID_REQUEST = 1U,
    TIMEOUT = 2U,
    INTERNAL_ERROR = 3U,
};

struct StatusData {
    float vbat;
    float ibat;
    float soc;
    float vsolar;
    float isolar;
    float temp_bat;
    float power_out;
    std::uint8_t pdu_status;
    std::uint8_t sunlight;
    std::uint8_t heater_enabled;
    std::uint8_t overcurrent_flags;
};

}  // namespace EPS
}  // namespace OBC

#endif
