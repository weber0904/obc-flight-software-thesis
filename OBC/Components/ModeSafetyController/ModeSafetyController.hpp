#ifndef OBC_COMPONENTS_MODESAFETYCONTROLLER_MODESAFETYCONTROLLER_HPP
#define OBC_COMPONENTS_MODESAFETYCONTROLLER_MODESAFETYCONTROLLER_HPP

#include "OBC/Components/ModeSafetyController/ModeSafetyControllerComponentAc.hpp"
#include "OBC/Components/ModeSafetyController/ModeSafetyPolicy.hpp"
#include "OBC/Components/ModeSafetyController/ModeSafetyRuntime.hpp"
#include "simulators/eps/EpsTypes.hpp"

namespace OBC {

class ModeSafetyController final : public ModeSafetyControllerComponentBase,
                                   public OBC::IModeOperatorTransitionGuard {
  public:
    explicit ModeSafetyController(const char* const compName);

    ~ModeSafetyController() override;

    void configureRuntime(OBC::IModeSafetyModeControl* modeControl, const OBC::IModeSafetyEpsStatus* epsStatus);

    OBC::OperatorModeTransitionDecision evaluateOperatorTransition(OBC::SatMode currentMode,
                                                                   OBC::SatMode requestedMode) const override;

    OBC::ModeSafetyDecision runCycle();

  private:
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    void publishDecision_(const OBC::ModeSafetyDecision& decision);

  private:
    OBC::IModeSafetyModeControl* m_modeControl;
    const OBC::IModeSafetyEpsStatus* m_epsStatus;
    U32 m_transitionCount;
    bool m_epsUnavailableLatched;
};

}  // namespace OBC

#endif
