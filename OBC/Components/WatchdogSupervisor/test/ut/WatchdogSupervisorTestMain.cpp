#include "WatchdogSupervisorTester.hpp"

TEST(WatchdogSupervisor, ResourceMonitoringMigration) {
    OBC::WatchdogSupervisorTester tester;
    tester.testResourceMonitoringMigration();
}

TEST(WatchdogSupervisor, ResourceMonitoringWarningsOnlyOnThresholdCrossing) {
    OBC::WatchdogSupervisorTester tester;
    tester.testResourceMonitoringWarningsOnlyOnThresholdCrossing();
}

TEST(WatchdogSupervisor, ResourceMonitoringEnableReplaysActiveThresholdState) {
    OBC::WatchdogSupervisorTester tester;
    tester.testResourceMonitoringEnableReplaysActiveThresholdState();
}

TEST(WatchdogSupervisor, HealthyBeatsKeepFeedEligible) {
    OBC::WatchdogSupervisorTester tester;
    tester.testHealthyBeatsKeepFeedEligible();
}

TEST(WatchdogSupervisor, FaultEscalationAndRecovery) {
    OBC::WatchdogSupervisorTester tester;
    tester.testFaultEscalationAndRecovery();
}

TEST(WatchdogSupervisor, FaultLatchedInSafeDefersSafeUntilRequestableMode) {
    OBC::WatchdogSupervisorTester tester;
    tester.testFaultLatchedInSafeDefersSafeUntilRequestableMode();
}

TEST(WatchdogSupervisor, FeedStrokeAttemptRequiresConnectedFeedPort) {
    OBC::WatchdogSupervisorTester tester(false);
    tester.testFeedStrokeAttemptRequiresConnectedFeedPort();
}

TEST(WatchdogSupervisor, ProbeSuppressionCommandUpdatesRuntimeState) {
    OBC::WatchdogSupervisorTester tester;
    tester.testProbeSuppressionCommandUpdatesRuntimeState();
}

TEST(WatchdogSupervisor, ProbeSuppressionImmediatelyFeedsSuppressedState) {
    OBC::WatchdogSupervisorTester tester;
    tester.testProbeSuppressionImmediatelyFeedsSuppressedState();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
