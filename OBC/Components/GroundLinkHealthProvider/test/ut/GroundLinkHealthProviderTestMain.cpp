#include "GroundLinkHealthProviderTester.hpp"

TEST(Nominal, CommCspStatusObservationKeepsAvailabilityHealthy) {
    OBC::GroundLinkHealthProviderTester tester;
    tester.testCommCspStatusObservationKeepsAvailabilityHealthy();
}

TEST(Nominal, CommCspStaleTransitionAndRecovery) {
    OBC::GroundLinkHealthProviderTester tester;
    tester.testCommCspStaleTransitionAndRecovery();
}

TEST(Nominal, CommCspStaleAgeTelemetryKeepsAdvancing) {
    OBC::GroundLinkHealthProviderTester tester;
    tester.testCommCspStaleAgeTelemetryKeepsAdvancing();
}

TEST(Nominal, MarkTelemetryDirtyRepublishesActivityAgeTelemetry) {
    OBC::GroundLinkHealthProviderTester tester;
    tester.testMarkTelemetryDirtyRepublishesActivityAgeTelemetry();
}

TEST(Nominal, ErrorGrowthTracksByCycle) {
    OBC::GroundLinkHealthProviderTester tester;
    tester.testErrorGrowthTracksByCycle();
}

TEST(Nominal, ConnectedOnlyCompatibilitySuppressesTransportGrowthAndStale) {
    OBC::GroundLinkHealthProviderTester tester;
    tester.testConnectedOnlyCompatibilitySuppressesTransportGrowthAndStale();
}

TEST(Nominal, DirectTcpConnectedFallbackDoesNotGoStale) {
    OBC::GroundLinkHealthProviderTester tester;
    tester.testDirectTcpConnectedFallbackDoesNotGoStale();
}
