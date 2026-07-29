#ifndef OBC_AdcsBridgeTester_HPP
#define OBC_AdcsBridgeTester_HPP

#include "OBC/Components/AdcsBridge/AdcsBridge.hpp"
#include "OBC/Components/AdcsBridge/AdcsBridgeGTestBase.hpp"

namespace OBC {

class AdcsBridgeTester final : public AdcsBridgeGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 48;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    AdcsBridgeTester();

    ~AdcsBridgeTester() override;

    void testGetAttitudePublishesExplicitRefreshTelemetry();
    void testGetAttitudeRepublishesExplicitRefreshWhenValuesUnchanged();

    void testSetModeCanTriggerDetumbleCompletion();

    void testPointingAcquiredAfterTargetUpdate();

    void testScheduledPollPublishesContinuousOnly();
    void testScheduledPollPublishesChangeDrivenTelemetryOnModeChange();
    void testScheduledPollThrottlesContinuousTelemetry();
    void testSchedInUsesAsyncOwnerFlow();
    void testSchedInHonorsConfiguredPollPeriod();

    void testInvalidSensorReplyPreservesLastTelemetry();

    void testTimeoutPreservesLastTelemetry();

    void testCommandFailureDoesNotMutateScheduledPollHealth();

    void testRecoveryResetUpdatesCachedState();

    void testRecoveryResetFailureDoesNotMutateScheduledPollHealth();

  private:
    void connectPorts();

    void initComponents();

  protected:
    void from_adcsStatusRefreshTlmOut_handler(FwIndexType portNum,
                                              FwChanIdType id,
                                              Fw::Time& timeTag,
                                              Fw::TlmBuffer& val) override;

  private:
    OBC::AdcsBridge component;
};

}  // namespace OBC

#endif
