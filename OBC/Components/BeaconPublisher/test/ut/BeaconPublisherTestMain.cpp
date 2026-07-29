#include "BeaconPublisherTester.hpp"

TEST(BeaconPublisher, PublishEmitsPacket) {
    OBC::BeaconPublisherTester tester;
    tester.testPublishEmitsPacket();
}

TEST(BeaconPublisher, CadenceWaitsBetweenPackets) {
    OBC::BeaconPublisherTester tester;
    tester.testCadenceWaitsBetweenPackets();
}

TEST(BeaconPublisher, UnavailableSourceReportsError) {
    OBC::BeaconPublisherTester tester;
    tester.testUnavailableSourceReportsError();
}

TEST(BeaconPublisher, NotConfiguredReportsDistinctError) {
    OBC::BeaconPublisherTester tester;
    tester.testNotConfiguredReportsDistinctError();
}

TEST(BeaconPublisher, DisabledSchedulerIsSilent) {
    OBC::BeaconPublisherTester tester;
    tester.testDisabledSchedulerIsSilent();
}

TEST(BeaconPublisher, SinkFailureReportsError) {
    OBC::BeaconPublisherTester tester;
    tester.testSinkFailureReportsError();
}

TEST(BeaconPublisher, RuntimeSuppressIsSilent) {
    OBC::BeaconPublisherTester tester;
    tester.testRuntimeSuppressIsSilent();
}

TEST(BeaconPublisher, RuntimeSuppressClearResumesOnNextTick) {
    OBC::BeaconPublisherTester tester;
    tester.testRuntimeSuppressClearResumesOnNextTick();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
