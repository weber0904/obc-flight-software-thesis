#ifndef OBC_COMPONENTS_MODESAFETYCONTROLLER_MODESAFETYPOLICY_HPP
#define OBC_COMPONENTS_MODESAFETYCONTROLLER_MODESAFETYPOLICY_HPP

#include "Fw/Types/BasicTypes.hpp"
#include "OBC/Types/SatModeEnumAc.hpp"

namespace OBC {

enum class ModeSafetyAction : U8 {
    NONE = 0,
    SAFE_TO_HELL = 1,
    HELL_TO_SAFE = 2,
    ACTIVE_TO_SAFE = 3,
    PAYLOAD_TO_IDLE = 4,
};

struct ModeSafetyDecision {
    bool hasValidEpsStatus = false;
    OBC::SatMode currentMode = OBC::SatMode::SAFE;
    OBC::SatMode targetMode = OBC::SatMode::SAFE;
    F32 soc = 0.0F;
    ModeSafetyAction action = ModeSafetyAction::NONE;

    bool shouldTransition() const;
};

class ModeSafetyPolicy final {
  public:
    static constexpr F32 SAFE_TO_HELL_SOC = 10.0F;
    static constexpr F32 HELL_TO_SAFE_SOC = 15.0F;
    static constexpr F32 ACTIVE_TO_SAFE_SOC = 40.0F;
    static constexpr F32 SAFE_TO_IDLE_SOC = 50.0F;
    static constexpr F32 PAYLOAD_TO_IDLE_SOC = 60.0F;
    static constexpr F32 IDLE_TO_PAYLOAD_SOC = 70.0F;

    static ModeSafetyDecision evaluate(OBC::SatMode currentMode, bool hasValidEpsStatus, F32 soc);

  private:
    static bool isModeWithSafeFallbackFloor_(OBC::SatMode mode);
};

}  // namespace OBC

#endif
