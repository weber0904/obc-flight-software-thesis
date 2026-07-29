#include "RecoveryExecutorTester.hpp"

TEST(RecoveryExecutor, WatchdogFaultQueuesProcessRestartAndClears) {
    OBC::RecoveryExecutorTester tester;
    tester.testWatchdogFaultQueuesProcessRestartAndClears();
}

TEST(RecoveryExecutor, WatchdogSuppressionEscalatesToReboot) {
    OBC::RecoveryExecutorTester tester;
    tester.testWatchdogSuppressionEscalatesToReboot();
}

TEST(RecoveryExecutor, WatchdogSuppressionWithHardwareWatchdogLeavesPendingRebootWithoutRuntimeExit) {
    OBC::RecoveryExecutorTester tester;
    tester.testWatchdogSuppressionWithHardwareWatchdogLeavesPendingRebootWithoutRuntimeExit();
}

TEST(RecoveryExecutor, WatchdogFaultInHardwareWatchdogModeStillQueuesProcessRestart) {
    OBC::RecoveryExecutorTester tester;
    tester.testWatchdogFaultInHardwareWatchdogModeStillQueuesProcessRestart();
}

TEST(RecoveryExecutor, EpsFaultResetThenTimeoutEscalatesToReboot) {
    OBC::RecoveryExecutorTester tester;
    tester.testEpsFaultResetThenTimeoutEscalatesToReboot();
}

TEST(RecoveryExecutor, NonWatchdogRebootStillRequestsRuntimeExitWhenHardwareWatchdogEnabled) {
    OBC::RecoveryExecutorTester tester;
    tester.testNonWatchdogRebootStillRequestsRuntimeExitWhenHardwareWatchdogEnabled();
}

TEST(RecoveryExecutor, ExistingRuntimeRebootRequestSurvivesWatchdogRebootIntent) {
    OBC::RecoveryExecutorTester tester;
    tester.testExistingRuntimeRebootRequestSurvivesWatchdogRebootIntent();
}

TEST(RecoveryExecutor, BootSafeFallbackAndStableAck) {
    OBC::RecoveryExecutorTester tester;
    tester.testBootSafeFallbackAndStableAck();
}

TEST(RecoveryExecutor, BootSafeFallbackClampSuppressesR2RestartLoop) {
    OBC::RecoveryExecutorTester tester;
    tester.testBootSafeFallbackClampSuppressesR2RestartLoop();
}

TEST(RecoveryExecutor, BootSafeFallbackClampAlreadySafeQueuesSafeAction) {
    OBC::RecoveryExecutorTester tester;
    tester.testBootSafeFallbackClampAlreadySafeQueuesSafeAction();
}

TEST(RecoveryExecutor, GetRecoveryStatusCommandReportsState) {
    OBC::RecoveryExecutorTester tester;
    tester.testGetRecoveryStatusCommandReportsState();
}

TEST(RecoveryExecutor, RebootRequestWaitsForBootMetadataPersistence) {
    OBC::RecoveryExecutorTester tester;
    tester.testRebootRequestWaitsForBootMetadataPersistence();
}

TEST(RecoveryExecutor, ProcessRestartPersistenceFailureFallsBackToSafe) {
    OBC::RecoveryExecutorTester tester;
    tester.testProcessRestartPersistenceFailureFallsBackToSafe();
}

TEST(RecoveryExecutor, StableAckFailureRetriesAfterPersistenceReturns) {
    OBC::RecoveryExecutorTester tester;
    tester.testStableAckFailureRetriesAfterPersistenceReturns();
}

TEST(RecoveryExecutor, ClearDoesNotDropPendingRebootBeforeConsume) {
    OBC::RecoveryExecutorTester tester;
    tester.testClearDoesNotDropPendingRebootBeforeConsume();
}

TEST(RecoveryExecutor, AdcsFaultQueuesResetWithoutProcessRestart) {
    OBC::RecoveryExecutorTester tester;
    tester.testAdcsFaultQueuesResetWithoutProcessRestart();
}

TEST(RecoveryExecutor, AdcsRelatchEscalatesThroughExistingRebootPath) {
    OBC::RecoveryExecutorTester tester;
    tester.testAdcsRelatchEscalatesThroughExistingRebootPath();
}

TEST(RecoveryExecutor, CommFailoverRunsOutsideExecutorLock) {
    OBC::RecoveryExecutorTester tester;
    tester.testCommFailoverRunsOutsideExecutorLock();
}

TEST(RecoveryExecutor, PersistentFaultLifecycleBreadcrumbs) {
    OBC::RecoveryExecutorTester tester;
    tester.testPersistentFaultLifecycleBreadcrumbs();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
