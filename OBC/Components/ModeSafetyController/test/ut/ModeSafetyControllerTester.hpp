#ifndef OBC_COMPONENTS_MODESAFETYCONTROLLER_TEST_UT_MODESAFETYCONTROLLERTESTER_HPP
#define OBC_COMPONENTS_MODESAFETYCONTROLLER_TEST_UT_MODESAFETYCONTROLLERTESTER_HPP

#include "OBC/Components/ModeSafetyController/ModeSafetyController.hpp"
#include "OBC/Components/ModeSafetyController/ModeSafetyControllerGTestBase.hpp"

namespace OBC {

class ModeSafetyControllerTester final : public ModeSafetyControllerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    ModeSafetyControllerTester();

    ~ModeSafetyControllerTester() override;

    void testNoCachedEpsStatusDoesNotTransition();

    void testUnconfiguredRuntimeDoesNotEvaluate();

    void testSafeToHellThresholdIsStrict();

    void testHellToSafeThresholdIsStrict();

    void testActiveModesToSafeThresholdIsStrict();

    void testPayloadToIdleThresholdAndPriority();

    void testAlreadyTargetAndHighSocSafeAreNoOps();

    void testOperatorGuardMatrix();

    void testOperatorHellToSafeGuardUsesCachedEps();

    void testOperatorAdmissionGuardsUseCachedEps();

  private:
    class FakeModeControl final : public OBC::IModeSafetyModeControl {
      public:
        OBC::SatMode getModeForRuntime() const override;

        void applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) override;

        OBC::SatMode mode = OBC::SatMode::SAFE;
        U32 transitionCount = 0U;
        OBC::SatMode lastRequestedMode = OBC::SatMode::SAFE;
        OBC::ModeApplySource lastSource = OBC::ModeApplySource::TestSetup;
    };

    class FakeEpsStatus final : public OBC::IModeSafetyEpsStatus {
      public:
        bool getCachedStatusForRuntime(OBC::EPS::StatusData& status) const override;

        bool available = true;
        OBC::EPS::StatusData status = {};
    };

    void connectPorts();

    void initComponents();

    void configure(OBC::SatMode mode, bool epsAvailable, F32 soc);

  private:
    FakeModeControl m_modeControl;
    FakeEpsStatus m_epsStatus;
    OBC::ModeSafetyController component;
};

}  // namespace OBC

#endif
