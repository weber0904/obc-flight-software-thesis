#ifndef OBC_COMPONENTS_RECOVERYEXECUTOR_TEST_UT_RECOVERYEXECUTORTESTER_HPP
#define OBC_COMPONENTS_RECOVERYEXECUTOR_TEST_UT_RECOVERYEXECUTORTESTER_HPP

#include <string>
#include <vector>

#include "OBC/Components/AdcsBridge/AdcsBridge.hpp"
#include "OBC/Components/BootManager/BootManager.hpp"
#include "OBC/Components/EpsBridge/EpsBridge.hpp"
#include "OBC/Components/PersistentFaultManager/PersistentFaultRuntime.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryExecutor.hpp"
#include "OBC/Components/RecoveryExecutor/RecoveryExecutorGTestBase.hpp"
#include "simulators/adcs/AdcsTransport.hpp"
#include "simulators/eps/EpsTransport.hpp"

namespace OBC {

class RecoveryExecutorTester final : public RecoveryExecutorGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 64;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    RecoveryExecutorTester();

    ~RecoveryExecutorTester() override;

    void testWatchdogFaultQueuesProcessRestartAndClears();

    void testWatchdogSuppressionEscalatesToReboot();

    void testWatchdogSuppressionWithHardwareWatchdogLeavesPendingRebootWithoutRuntimeExit();

    void testWatchdogFaultInHardwareWatchdogModeStillQueuesProcessRestart();

    void testEpsFaultResetThenTimeoutEscalatesToReboot();

    void testNonWatchdogRebootStillRequestsRuntimeExitWhenHardwareWatchdogEnabled();

    void testExistingRuntimeRebootRequestSurvivesWatchdogRebootIntent();

    void testBootSafeFallbackAndStableAck();

    void testBootSafeFallbackClampSuppressesR2RestartLoop();

    void testBootSafeFallbackClampAlreadySafeQueuesSafeAction();

    void testGetRecoveryStatusCommandReportsState();

    void testRebootRequestWaitsForBootMetadataPersistence();

    void testProcessRestartPersistenceFailureFallsBackToSafe();

    void testStableAckFailureRetriesAfterPersistenceReturns();

    void testClearDoesNotDropPendingRebootBeforeConsume();

    void testAdcsFaultQueuesResetWithoutProcessRestart();

    void testAdcsRelatchEscalatesThroughExistingRebootPath();

    void testCommFailoverRunsOutsideExecutorLock();

    void testPersistentFaultLifecycleBreadcrumbs();

  private:
    class FakeModeControl final : public OBC::IModeSafetyModeControl {
      public:
        OBC::SatMode getModeForRuntime() const override { return this->currentMode; }
        void applyModeForInternalSource(OBC::SatMode mode, OBC::ModeApplySource source) override;

        OBC::SatMode currentMode = OBC::SatMode::IDLE;
        OBC::SatMode requestedMode = OBC::SatMode::IDLE;
        OBC::ModeApplySource lastSource = OBC::ModeApplySource::TestSetup;
        U32 applyCount = 0U;
    };

    class FakeEpsTransport final : public OBC::EPS::IEpsTransport {
      public:
        OBC::EPS::TransportStatus getStatus(OBC::EPS::StatusData& outStatus) override;
        OBC::EPS::TransportStatus setPdu(std::uint8_t channel,
                                         bool enabled,
                                         OBC::EPS::StatusData& outStatus) override;
        OBC::EPS::TransportStatus setHeater(bool enabled, OBC::EPS::StatusData& outStatus) override;
        OBC::EPS::TransportStatus reset(OBC::EPS::StatusData& outStatus) override;

        OBC::EPS::TransportStatus resetStatus = OBC::EPS::TransportStatus::OK;
        OBC::EPS::StatusData lastStatus = {};
        U32 resetCalls = 0U;
    };

    class FakeAdcsTransport final : public OBC::ADCS::IAdcsTransport {
      public:
        OBC::ADCS::TransportStatus getState(OBC::ADCS::StateData& outState) override;
        OBC::ADCS::TransportStatus setMode(std::uint8_t mode, OBC::ADCS::StateData& outState) override;
        OBC::ADCS::TransportStatus setTarget(double q0,
                                             double q1,
                                             double q2,
                                             double q3,
                                             OBC::ADCS::StateData& outState) override;
        OBC::ADCS::TransportStatus calibrate(std::uint8_t sensorId, OBC::ADCS::StateData& outState) override;
        OBC::ADCS::TransportStatus reset(OBC::ADCS::StateData& outState) override;

        OBC::ADCS::TransportStatus resetStatus = OBC::ADCS::TransportStatus::OK;
        OBC::ADCS::StateData lastState = {};
        U32 resetCalls = 0U;
    };

    class FakeCommControl final : public OBC::IRecoveryCommControl {
      public:
        OBC::RecoveryCommActionResult performRecoveryLinkFailoverForRuntime() override;

        OBC::RecoveryExecutor* owner = nullptr;
        OBC::RecoveryRuntimeStatus lastObservedStatus = {};
        OBC::RecoveryCommActionResult nextResult = {};
        U32 actionCount = 0U;
    };

    class FakePersistentFaultRecorder final : public OBC::IPersistentFaultRecorder {
      public:
        bool appendPersistentFaultRecordForRuntime(const OBC::PersistentFaultRecord& record) override {
            this->records.push_back(record);
            return true;
        }

        std::vector<OBC::PersistentFaultRecord> records = {};
    };

    void connectPorts();

    void initComponents();

    std::string makeTempRoot_() const;

  private:
    std::string m_tempRoot;
    FakeModeControl m_modeControl;
    FakeEpsTransport m_epsTransport;
    FakeAdcsTransport m_adcsTransport;
    FakeCommControl m_commControl;
    FakePersistentFaultRecorder m_faultRecorder;
    OBC::BootManager m_bootManager;
    OBC::AdcsBridge m_adcsBridge;
    OBC::EpsBridge m_epsBridge;
    OBC::RecoveryExecutor component;
};

}  // namespace OBC

#endif
