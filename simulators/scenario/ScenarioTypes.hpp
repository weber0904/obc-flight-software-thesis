#ifndef OBC_SIMULATORS_SCENARIO_SCENARIOTYPES_HPP
#define OBC_SIMULATORS_SCENARIO_SCENARIOTYPES_HPP

#include <cstdint>

namespace OBC {
namespace Scenario {

struct ReplaySample {
    double time_sec;
    std::uint8_t sunlight;
    float battery_soc_pct;
    std::uint8_t ground_pass_open;
    std::uint8_t link_available;
    float omega_x_rad_s;
    float omega_y_rad_s;
    float omega_z_rad_s;
};

}  // namespace Scenario
}  // namespace OBC

#endif
