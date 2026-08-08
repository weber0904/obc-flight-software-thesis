#ifndef OBC_EpsBridgeTester_HPP
#define OBC_EpsBridgeTester_HPP

#include "OBC/Components/EpsBridge/EpsBridge.hpp"
#include "OBC/Components/EpsBridge/EpsBridgeGTestBase.hpp"

namespace OBC {

class EpsBridgeTester final : public EpsBridgeGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    EpsBridgeTester();

    ~EpsBridgeTester() override;

    void testGetStatusPublishesExplicitRefreshTelemetry();

    void testPduCommandPublishesChangeEventAndExplicitRefresh();

    void testHeaterCommandPublishesExplicitRefreshTelemetry();

    void testResetPublishesExplicitRefreshTelemetry();

    void testLowBatteryAndCriticalEvents();

    void testScheduledPollPublishesContinuousOnly();
    void testScheduledPollPublishesChangeDrivenTelemetryOnStateChange();
    void testScheduledPollThrottlesContinuousTelemetry();
    void testSchedInUsesAsyncOwnerFlow();
    void testSchedInHonorsConfiguredPollPeriod();

    void testTimeoutInvalidatesCachedStatusAndPreservesLastTelemetry();

    void testPollHealthTracksConsecutiveFailuresAndRecovery();

    void testRuntimeFetchDoesNotMutatePollHealth();

    void testPollHealthCountersSaturateAtMax();

  private:
    void connectPorts();

    void initComponents();

  protected:
    void from_epsStatusRefreshTlmOut_handler(FwIndexType portNum,
                                             FwChanIdType id,
                                             Fw::Time& timeTag,
                                             Fw::TlmBuffer& val) override;

  private:
    OBC::EpsBridge component;
};

}  // namespace OBC

#endif
