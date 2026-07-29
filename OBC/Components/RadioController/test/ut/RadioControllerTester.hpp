#ifndef OBC_RadioControllerTester_HPP
#define OBC_RadioControllerTester_HPP

#include "OBC/Components/RadioController/RadioController.hpp"
#include "OBC/Components/RadioController/RadioControllerGTestBase.hpp"

namespace OBC {

class RadioControllerTester final : public RadioControllerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    RadioControllerTester();

    ~RadioControllerTester() override;

    void testGetStatusPublishesTelemetry();

    void testEnablePublishesPowerEvent();

    void testOvertempRaisesEvent();

    void testScheduledPollPublishesSummaryOnly();

    void testInvalidReplyMapsToValidationError();

    void testCachedObservationAgesWhenPollFails();

    void testUnavailableObservationWithoutSample();

    void testTransportErrorUpdatesCachedObservation();

    void testUnsupportedProtocolUpdatesObservation();

    void testSetterFailureUpdatesCachedObservation();

  private:
    void connectPorts();

    void initComponents();

  private:
    OBC::RadioController component;
};

}  // namespace OBC

#endif
