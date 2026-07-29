#ifndef OBC_COMPONENTS_MISSIONEXECUTIVE_HPP
#define OBC_COMPONENTS_MISSIONEXECUTIVE_HPP

#include "OBC/Components/MissionExecutive/MissionExecutiveComponentAc.hpp"
#include "OBC/Components/MissionExecutive/MissionExecutivePolicy.hpp"

namespace OBC {

class MissionExecutive final : public MissionExecutiveComponentBase {
  public:
    explicit MissionExecutive(const char* const compName);

    ~MissionExecutive() override;

    void configureRuntimeBindings(ILowPowerModeControl* modeControl,
                                  IEpsAutonomyStatus* epsStatus,
                                  IAdcsAutonomyStatus* adcsStatus,
                                  IAdcsDetumbleControl* adcsDetumbleControl,
                                  IAdcsSunPointingControl* adcsControl);

    MissionExecutiveStepResult runCycleForTest();

  private:
    void schedIn_handler(const FwIndexType portNum, U32 context) override;

    void publishStepResult_(const MissionExecutiveStepResult& result);

  private:
    MissionExecutivePolicy m_policy;
};

}  // namespace OBC

#endif
