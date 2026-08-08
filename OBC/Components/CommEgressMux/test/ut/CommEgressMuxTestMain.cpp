#include "CommEgressMuxTester.hpp"

TEST(CommEgressMux, DefaultsSuppressSbandUntilEnabled) {
    OBC::CommEgressMuxTester tester;
    tester.testDefaultsSuppressSbandUntilEnabled();
}

TEST(CommEgressMux, SbandLiveObservabilityRoutesAfterEnable) {
    OBC::CommEgressMuxTester tester;
    tester.testSbandLiveObservabilityRoutesAfterEnable();
}

TEST(CommEgressMux, SwitchRoutesTelemetryAndFileToUhf) {
    OBC::CommEgressMuxTester tester;
    tester.testSwitchRoutesTelemetryAndFileToUhf();
}

TEST(CommEgressMux, UhfFileTransferSuspendsAllPacketEgress) {
    OBC::CommEgressMuxTester tester;
    tester.testUhfFileTransferSuspendsAllPacketEgress();
}

TEST(CommEgressMux, QuietModeSuppressesPacketEgress) {
    OBC::CommEgressMuxTester tester;
    tester.testQuietModeSuppressesPacketEgress();
}

TEST(CommEgressMux, QuietModeSuppressesSbandPacketEgress) {
    OBC::CommEgressMuxTester tester;
    tester.testQuietModeSuppressesSbandPacketEgress();
}

TEST(CommEgressMux, QuietModeReturnsFileBuffersLocally) {
    OBC::CommEgressMuxTester tester;
    tester.testQuietModeReturnsFileBuffersLocally();
}

TEST(CommEgressMux, UhfPrimaryPacketQuietSuppressesPacketEgressOnly) {
    OBC::CommEgressMuxTester tester;
    tester.testUhfPrimaryPacketQuietSuppressesPacketEgressOnly();
}

TEST(CommEgressMux, UhfRouteCountersTrackEventAndTelemetryPackets) {
    OBC::CommEgressMuxTester tester;
    tester.testUhfRouteCountersTrackEventAndTelemetryPackets();
}

TEST(CommEgressMux, UhfSuppressCountersTrackQuietPackets) {
    OBC::CommEgressMuxTester tester;
    tester.testUhfSuppressCountersTrackQuietPackets();
}

TEST(CommEgressMux, SbandCountersTrackSuppressedAndRoutedPackets) {
    OBC::CommEgressMuxTester tester;
    tester.testSbandCountersTrackSuppressedAndRoutedPackets();
}

TEST(CommEgressMux, TelemetryLinkSwitchRepublishesUhfCounters) {
    OBC::CommEgressMuxTester tester;
    tester.testTelemetryLinkSwitchRepublishesUhfCounters();
}

TEST(CommEgressMux, ReturnPathMergesBothBranches) {
    OBC::CommEgressMuxTester tester;
    tester.testReturnPathMergesBothBranches();
}
