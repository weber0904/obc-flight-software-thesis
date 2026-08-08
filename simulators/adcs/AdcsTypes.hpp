#ifndef OBC_SIMULATORS_ADCS_ADCSTYPES_HPP
#define OBC_SIMULATORS_ADCS_ADCSTYPES_HPP

#include <cstdint>

namespace OBC {
namespace ADCS {

enum class ResultCode : std::uint8_t {
    OK = 0U,
    INVALID_REQUEST = 1U,
    TIMEOUT = 2U,
    INTERNAL_ERROR = 3U,
};

struct StateData {
    double q0;
    double q1;
    double q2;
    double q3;
    float omega_x;
    float omega_y;
    float omega_z;
    float mag_x;
    float mag_y;
    float mag_z;
    float pointing_error_deg;
    std::uint8_t mode;
    std::uint8_t sensor_valid;
    std::uint16_t reserved;
};

}  // namespace ADCS
}  // namespace OBC

#endif
