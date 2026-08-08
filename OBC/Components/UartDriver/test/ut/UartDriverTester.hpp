#ifndef OBC_UartDriverTester_HPP
#define OBC_UartDriverTester_HPP

#include "OBC/Components/UartDriver/UartDriver.hpp"
#include "OBC/Components/UartDriver/UartDriverGTestBase.hpp"

namespace OBC {

class UartDriverTester final : public UartDriverGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 20;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    UartDriverTester();

    ~UartDriverTester() override;

    void testConnectPublishesOpenAndTelemetry();

    void testExchangeUpdatesCounters();

    void testTimeoutPublishesError();

    void testRuntimeSendReportsTransportResult();

    void testRuntimeSendFailsWithoutTransport();

    void testRuntimeSendFailurePublishesError();

    void testQueuedRuntimeSendDrainsOnPoll();

    void testQueuedRuntimeSendRejectsDisconnectedTransport();

    void testQueuedRuntimeSendRejectsFullQueue();

  private:
    void connectPorts();

    void initComponents();

  private:
    OBC::UartDriver component;
};

}  // namespace OBC

#endif
