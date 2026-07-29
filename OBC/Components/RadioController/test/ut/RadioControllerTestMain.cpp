#include "RadioControllerTester.hpp"

TEST(RadioController, GetStatusPublishesTelemetry) {
    OBC::RadioControllerTester tester;
    tester.testGetStatusPublishesTelemetry();
}

TEST(RadioController, EnablePublishesPowerEvent) {
    OBC::RadioControllerTester tester;
    tester.testEnablePublishesPowerEvent();
}

TEST(RadioController, OvertempRaisesEvent) {
    OBC::RadioControllerTester tester;
    tester.testOvertempRaisesEvent();
}

TEST(RadioController, ScheduledPollPublishesSummaryOnly) {
    OBC::RadioControllerTester tester;
    tester.testScheduledPollPublishesSummaryOnly();
}

TEST(RadioController, InvalidReplyMapsToValidationError) {
    OBC::RadioControllerTester tester;
    tester.testInvalidReplyMapsToValidationError();
}

TEST(RadioController, CachedObservationAgesWhenPollFails) {
    OBC::RadioControllerTester tester;
    tester.testCachedObservationAgesWhenPollFails();
}

TEST(RadioController, UnavailableObservationWithoutSample) {
    OBC::RadioControllerTester tester;
    tester.testUnavailableObservationWithoutSample();
}

TEST(RadioController, TransportErrorUpdatesCachedObservation) {
    OBC::RadioControllerTester tester;
    tester.testTransportErrorUpdatesCachedObservation();
}

TEST(RadioController, UnsupportedProtocolUpdatesObservation) {
    OBC::RadioControllerTester tester;
    tester.testUnsupportedProtocolUpdatesObservation();
}

TEST(RadioController, SetterFailureUpdatesCachedObservation) {
    OBC::RadioControllerTester tester;
    tester.testSetterFailureUpdatesCachedObservation();
}
