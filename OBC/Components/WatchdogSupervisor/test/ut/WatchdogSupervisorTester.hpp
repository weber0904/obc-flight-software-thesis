#ifndef OBC_COMPONENTS_WATCHDOGSUPERVISOR_TEST_UT_WATCHDOGSUPERVISORTESTER_HPP
#define OBC_COMPONENTS_WATCHDOGSUPERVISOR_TEST_UT_WATCHDOGSUPERVISORTESTER_HPP

#include "OBC/Components/WatchdogSupervisor/WatchdogSupervisor.hpp"
#include "OBC/Components/WatchdogSupervisor/WatchdogSupervisorGTestBase.hpp"

namespace OBC {

class WatchdogSupervisorTester final : public WatchdogSupervisorGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 50;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    explicit WatchdogSupervisorTester(bool connectWatchdogFeedPort = true);

    ~WatchdogSupervisorTester() override;

    void testResourceMonitoringMigration();
    void testResourceMonitoringWarningsOnlyOnThresholdCrossing();
    void testResourceMonitoringEnableReplaysActiveThresholdState();

    void testHealthyBeatsKeepFeedEligible();

    void testFaultEscalationAndRecovery();

    void testFaultLatchedInSafeDefersSafeUntilRequestableMode();

    void testFeedStrokeAttemptRequiresConnectedFeedPort();

    void testProbeSuppressionCommandUpdatesRuntimeState();

    void testProbeSuppressionImmediatelyFeedsSuppressedState();

  private:
    class FakeModeControl final : public OBC::IModeSafetyModeControl {
      public:
        OBC::SatMode getModeForRuntime() const override { return this->currentMode; }
        void applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) override {
            this->requestedMode = mode;
            this->lastSource = source;
            this->applyCount++;
        }

        OBC::SatMode currentMode = OBC::SatMode::IDLE;
        OBC::SatMode requestedMode = OBC::SatMode::IDLE;
        OBC::ModeApplySource lastSource = OBC::ModeApplySource::TestSetup;
        U32 applyCount = 0U;
    };

    class FakeRecoverySink final : public OBC::IRecoveryRequestSink {
      public:
        void submitWatchdogFault(OBC::WatchdogSource source) override;
        void submitWatchdogSuppression(OBC::WatchdogSource source) override;
        void clearWatchdogFault(OBC::WatchdogSource source) override;
        void submitEpsTimeoutFault(U32 failureCount) override;
        void clearEpsTimeoutFault(U32 failureCount) override;
        void submitAdcsPollTransportFault(U32) override {}
        void clearAdcsPollTransportFault(U32) override {}
        void submitAdcsPollFreshnessFault(U32) override {}
        void clearAdcsPollFreshnessFault(U32) override {}
        void submitCommPrimaryUnavailableFault(U32) override {}
        void clearCommPrimaryUnavailableFault(U32) override {}
        void submitCommPrimaryTransportFault(U32) override {}
        void clearCommPrimaryTransportFault(U32) override {}

        U32 watchdogFaultCount = 0U;
        U32 watchdogSuppressionCount = 0U;
        U32 watchdogClearCount = 0U;
        OBC::WatchdogSource lastWatchdogSource = OBC::WatchdogSource::EPS_BRIDGE;
        U32 epsFaultCount = 0U;
        U32 epsClearCount = 0U;
        U32 lastEpsFailureCount = 0U;
    };

    void connectPorts();

    void initComponents();

    void connectComponentPorts_(bool connectWatchdogFeedPort);

    void tickAllHealthy_();

    void tickAllExcept_(OBC::WatchdogSource source);

    void from_watchdogFeedOut_handler(FwIndexType portNum, U32 code) override;

  private:
    OBC::WatchdogSupervisor component;
    FakeModeControl modeControl;
    FakeRecoverySink recoverySink;
    U32 feedStrokeCount;
    U32 lastFeedCode;
};

}  // namespace OBC

#endif
