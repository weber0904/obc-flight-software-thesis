#ifndef OBC_MISSIONEXECUTIVETESTER_HPP
#define OBC_MISSIONEXECUTIVETESTER_HPP

#include "OBC/Components/MissionExecutive/MissionExecutive.hpp"
#include "OBC/Components/MissionExecutive/MissionExecutiveGTestBase.hpp"

namespace OBC {

class MissionExecutiveTester final : public MissionExecutiveGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 10;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    MissionExecutiveTester();

    ~MissionExecutiveTester() override;

    void testHighAngularRateTriggersDetumble();

    void testHighRateSuppressesSunSafePointingAtLowBattery();

    void testLowBatteryTriggersLowPowerAndSunPointing();

    void testPolicyLatchesAfterEntry();

    void testNominalBatteryDoesNothing();

  private:
    void connectPorts();

    void initComponents();

  private:
    OBC::MissionExecutive component;
};

}  // namespace OBC

#endif
