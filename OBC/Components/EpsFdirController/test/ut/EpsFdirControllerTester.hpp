#ifndef OBC_COMPONENTS_EPSFDIRCONTROLLER_TEST_UT_EPSFDIRCONTROLLERTESTER_HPP
#define OBC_COMPONENTS_EPSFDIRCONTROLLER_TEST_UT_EPSFDIRCONTROLLERTESTER_HPP

#include "OBC/Components/EpsFdirController/EpsFdirController.hpp"
#include "OBC/Components/EpsFdirController/EpsFdirControllerGTestBase.hpp"

namespace OBC {

class EpsFdirControllerTester final : public EpsFdirControllerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    EpsFdirControllerTester();

    ~EpsFdirControllerTester() override;

    void testNoEscalationBeforeThreshold();

    void testFaultEscalatesOnceAtThreshold();

    void testRecoveryClearsFaultOnFirstSuccess();

    void testSafeAndHellRecordFaultWithoutModeRequest();

    void testCountersSaturateAtMax();

    void testRelatchReportsSecondFault();

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

    class FakeHealthProvider final : public OBC::IEpsFdirHealthProvider {
      public:
        bool getPollHealthForRuntime(OBC::EPS::PollHealthState& state) const override;

        bool available = true;
        OBC::EPS::PollHealthState state = {};
    };

    class FakeRecoverySink final : public OBC::IRecoveryRequestSink {
      public:
        void submitWatchdogFault(OBC::WatchdogSource) override {}
        void submitWatchdogSuppression(OBC::WatchdogSource) override {}
        void clearWatchdogFault(OBC::WatchdogSource) override {}
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

        U32 epsFaultCount = 0U;
        U32 epsClearCount = 0U;
        U32 lastFailureCount = 0U;
    };

    void connectPorts();

    void initComponents();

    void setHealth(bool lastPollSucceeded, U32 consecutiveFailures, bool cacheValid = false);

    void resetMode(OBC::SatMode mode);

  private:
    FakeModeControl m_modeControl;
    FakeHealthProvider m_healthProvider;
    FakeRecoverySink m_recoverySink;
    OBC::EpsFdirController component;
};

}  // namespace OBC

#endif
