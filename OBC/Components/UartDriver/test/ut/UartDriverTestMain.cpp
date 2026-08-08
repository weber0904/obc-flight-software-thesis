#include "UartDriverTester.hpp"

TEST(UartDriver, ConnectPublishesOpenAndTelemetry) {
    OBC::UartDriverTester tester;
    tester.testConnectPublishesOpenAndTelemetry();
}

TEST(UartDriver, ExchangeUpdatesCounters) {
    OBC::UartDriverTester tester;
    tester.testExchangeUpdatesCounters();
}

TEST(UartDriver, TimeoutPublishesError) {
    OBC::UartDriverTester tester;
    tester.testTimeoutPublishesError();
}

TEST(UartDriver, RuntimeSendReportsTransportResult) {
    OBC::UartDriverTester tester;
    tester.testRuntimeSendReportsTransportResult();
}

TEST(UartDriver, RuntimeSendFailsWithoutTransport) {
    OBC::UartDriverTester tester;
    tester.testRuntimeSendFailsWithoutTransport();
}

TEST(UartDriver, RuntimeSendFailurePublishesError) {
    OBC::UartDriverTester tester;
    tester.testRuntimeSendFailurePublishesError();
}

TEST(UartDriver, QueuedRuntimeSendDrainsOnPoll) {
    OBC::UartDriverTester tester;
    tester.testQueuedRuntimeSendDrainsOnPoll();
}

TEST(UartDriver, QueuedRuntimeSendRejectsDisconnectedTransport) {
    OBC::UartDriverTester tester;
    tester.testQueuedRuntimeSendRejectsDisconnectedTransport();
}

TEST(UartDriver, QueuedRuntimeSendRejectsFullQueue) {
    OBC::UartDriverTester tester;
    tester.testQueuedRuntimeSendRejectsFullQueue();
}
