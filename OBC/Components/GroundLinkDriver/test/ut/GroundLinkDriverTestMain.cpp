#include "GroundLinkDriverTester.hpp"

TEST(GroundLinkDriver, PumpPublishesReadyAndReceive) {
    OBC::GroundLinkDriverTester tester;
    tester.testPumpPublishesReadyAndReceive();
}

TEST(GroundLinkDriver, SendSuccessPublishesCounters) {
    OBC::GroundLinkDriverTester tester;
    tester.testSendSuccessPublishesCounters();
}

TEST(GroundLinkDriver, SendRetryReturnsRetry) {
    OBC::GroundLinkDriverTester tester;
    tester.testSendRetryReturnsRetry();
}

TEST(GroundLinkDriver, ReceiveErrorPublishesWarning) {
    OBC::GroundLinkDriverTester tester;
    tester.testReceiveErrorPublishesWarning();
}

TEST(GroundLinkDriver, RecvReturnDeallocates) {
    OBC::GroundLinkDriverTester tester;
    tester.testRecvReturnDeallocates();
}

TEST(GroundLinkDriver, ObserveHealthPublishesObservationState) {
    OBC::GroundLinkDriverTester tester;
    tester.testObserveHealthPublishesObservationState();
}

TEST(GroundLinkDriver, RuntimeObservationReadsCachedSnapshot) {
    OBC::GroundLinkDriverTester tester;
    tester.testRuntimeObservationReadsCachedSnapshot();
}
