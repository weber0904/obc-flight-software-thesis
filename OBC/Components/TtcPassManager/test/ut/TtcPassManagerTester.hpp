#ifndef OBC_COMPONENTS_TTCPASSMANAGER_TEST_UT_TTCPASSMANAGERTESTER_HPP
#define OBC_COMPONENTS_TTCPASSMANAGER_TEST_UT_TTCPASSMANAGERTESTER_HPP

#include "OBC/Components/TtcPassManager/TtcPassManager.hpp"
#include "OBC/Components/TtcPassManager/TtcPassManagerGTestBase.hpp"

namespace OBC {

class TtcPassManagerTester final : public TtcPassManagerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 64;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    TtcPassManagerTester();

    ~TtcPassManagerTester() override;

    void testConfigAndWindowCommands();

    void testGetStatusReplaysTelemetryWhenValuesUnchanged();

    void testSetPolicyReplaysTelemetryWhenValuesUnchanged();

    void testSchedInPublishesOnlyChangeDrivenOperatorStatus();

    void testInvalidWindowRejectedFailClosed();

    void testAutoEntryRequiresEnabledAndActiveWindow();

    void testAutoEntryTriggersAdcsPointingOnce();

    void testAdcsTriggerFailureDoesNotBlockTtcEntry();

    void testWindowInactiveExitsTtc();

    void testGpsInvalidExitsTtc();

    void testCommLossTimeoutExitsTtc();

    void testCommLossTimeoutUsesElapsedWallclockSeconds();

    void testCommLossTimeoutSuppressesReentryUntilWindowChanges();

    void testManualTtcStillSubjectToPolicy();

    void testManualIdleSuppressesReentryUntilWindowChanges();

    void testMidnightGpsSampleStillAllowsEntry();

    void testSafetyOverrideClearsLossTimerWithoutRestoringTtc();

  private:
    class FakeModeControl final : public OBC::IModeSafetyModeControl {
      public:
        OBC::SatMode getModeForRuntime() const override { return this->currentMode; }
        void applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) override;

        OBC::SatMode currentMode = OBC::SatMode::SAFE;
        OBC::SatMode requestedMode = OBC::SatMode::SAFE;
        OBC::ModeApplySource lastSource = OBC::ModeApplySource::TestSetup;
        U32 applyCount = 0U;
    };

    class FakeGpsProvider final : public OBC::ITtcPassGpsProvider {
      public:
        bool getCachedStateForRuntime(OBC::GPS::StateData& state) const override {
            state = this->state;
            return this->available;
        }

        bool available = true;
        OBC::GPS::StateData state = {};
    };

    class FakeCommProvider final : public OBC::ITtcPassCommStateProvider {
      public:
        OBC::CommRuntimeState getStateForRuntime() const override { return this->state; }

        OBC::CommRuntimeState state = {};
    };

    class FakeAdcsControl final : public OBC::ITtcPassAdcsControl {
      public:
        bool requestPointingForTtcEntry() override {
            this->requestCount++;
            return this->shouldSucceed;
        }

        bool shouldSucceed = true;
        U32 requestCount = 0U;
    };

    void connectPorts();

    void initComponents();

    void setTestTimeSec_(U32 seconds);

    void makeGpsValidAtEpoch_(U64 epochSec, U32 wallclockSec);

  private:
    OBC::TtcPassManager component;
    FakeModeControl modeControl;
    FakeGpsProvider gpsProvider;
    FakeCommProvider commProvider;
    FakeAdcsControl adcsControl;
};

}  // namespace OBC

#endif
