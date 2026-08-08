#include "OnboardStateMonitorTester.hpp"

TEST(OnboardStateMonitor, SchedPublishesReducedState) {
    OBC::OnboardStateMonitorTester tester;
    tester.testSchedPublishesReducedState();
}

TEST(OnboardStateMonitor, StateMonitorUpdatedRequiresMaskChange) {
    OBC::OnboardStateMonitorTester tester;
    tester.testStateMonitorUpdatedRequiresMaskChange();
}

TEST(OnboardStateMonitor, StateMonitorUpdatedRemainsQuietAcrossSourceRecovery) {
    OBC::OnboardStateMonitorTester tester;
    tester.testStateMonitorUpdatedRemainsQuietAcrossSourceRecovery();
}

TEST(OnboardStateMonitor, MissingSourceRaisesWarning) {
    OBC::OnboardStateMonitorTester tester;
    tester.testMissingSourceRaisesWarning();
}

TEST(OnboardStateMonitor, SourceFailureInvalidatesCachedState) {
    OBC::OnboardStateMonitorTester tester;
    tester.testSourceFailureInvalidatesCachedState();
}

TEST(OnboardStateMonitor, RecentReducedStateRingIsRuntimeReadable) {
    OBC::OnboardStateMonitorTester tester;
    tester.testRecentReducedStateRingIsRuntimeReadable();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
