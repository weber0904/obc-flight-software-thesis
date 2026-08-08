#ifndef OBC_COMPONENTS_ADCSFDIRCONTROLLER_TEST_UT_ADCSFDIRCONTROLLERTESTER_HPP
#define OBC_COMPONENTS_ADCSFDIRCONTROLLER_TEST_UT_ADCSFDIRCONTROLLERTESTER_HPP

#include "OBC/Components/AdcsFdirController/AdcsFdirController.hpp"
#include "OBC/Components/AdcsFdirController/AdcsFdirControllerGTestBase.hpp"

namespace OBC {

class AdcsFdirControllerTester final : public AdcsFdirControllerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 24;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    AdcsFdirControllerTester();

    ~AdcsFdirControllerTester() override;

    void testTransportRetriesBeforeLatch();

    void testFreshnessFaultLatchesAtThreshold();

    void testHealthyScheduledCycleClearsFault();

    void testLatchedFaultDoesNotSwitchSourceBeforeHealthyClear();

    void testSchedEmitsWatchdogBeat();

  private:
    class FakeHealthProvider final : public OBC::IAdcsFdirHealthProvider {
      public:
        bool getPollHealthForRuntime(OBC::ADCS::PollHealthState& state) const override;

        bool available = true;
        OBC::ADCS::PollHealthState state = {};
    };

    class FakeRecoverySink final : public OBC::IRecoveryRequestSink {
      public:
        void submitWatchdogFault(OBC::WatchdogSource) override {}
        void submitWatchdogSuppression(OBC::WatchdogSource) override {}
        void clearWatchdogFault(OBC::WatchdogSource) override {}
        void submitEpsTimeoutFault(U32) override {}
        void clearEpsTimeoutFault(U32) override {}
        void submitAdcsPollTransportFault(U32 failureCount) override;
        void clearAdcsPollTransportFault(U32 failureCount) override;
        void submitAdcsPollFreshnessFault(U32 failureCount) override;
        void clearAdcsPollFreshnessFault(U32 failureCount) override;
        void submitCommPrimaryUnavailableFault(U32) override {}
        void clearCommPrimaryUnavailableFault(U32) override {}
        void submitCommPrimaryTransportFault(U32) override {}
        void clearCommPrimaryTransportFault(U32) override {}

        U32 adcsTransportFaultCount = 0U;
        U32 adcsTransportClearCount = 0U;
        U32 adcsFreshnessFaultCount = 0U;
        U32 adcsFreshnessClearCount = 0U;
        U32 lastFailureCount = 0U;
    };

    void connectPorts();

    void initComponents();

    void setTransportHealth(U32 consecutiveFailures);

    void setFreshnessHealth(U32 consecutiveFailures);

    void setHealthyRefresh();

    void from_watchdogBeatOut_handler(FwIndexType portNum, U32 code) override;

  private:
    FakeHealthProvider m_healthProvider;
    FakeRecoverySink m_recoverySink;
    OBC::AdcsFdirController component;
    U32 m_watchdogBeatCount;
    U32 m_lastWatchdogCode;
};

}  // namespace OBC

#endif
