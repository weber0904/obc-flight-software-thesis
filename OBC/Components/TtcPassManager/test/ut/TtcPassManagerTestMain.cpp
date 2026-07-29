#include "TtcPassManagerTester.hpp"

#include <gtest/gtest.h>

TEST(TtcPassManager, ConfigAndWindowCommands) {
    OBC::TtcPassManagerTester tester;
    tester.testConfigAndWindowCommands();
}

TEST(TtcPassManager, GetStatusReplaysTelemetryWhenValuesUnchanged) {
    OBC::TtcPassManagerTester tester;
    tester.testGetStatusReplaysTelemetryWhenValuesUnchanged();
}

TEST(TtcPassManager, SetPolicyReplaysTelemetryWhenValuesUnchanged) {
    OBC::TtcPassManagerTester tester;
    tester.testSetPolicyReplaysTelemetryWhenValuesUnchanged();
}

TEST(TtcPassManager, SchedInPublishesOnlyChangeDrivenOperatorStatus) {
    OBC::TtcPassManagerTester tester;
    tester.testSchedInPublishesOnlyChangeDrivenOperatorStatus();
}

TEST(TtcPassManager, InvalidWindowRejectedFailClosed) {
    OBC::TtcPassManagerTester tester;
    tester.testInvalidWindowRejectedFailClosed();
}

TEST(TtcPassManager, AutoEntryRequiresEnabledAndActiveWindow) {
    OBC::TtcPassManagerTester tester;
    tester.testAutoEntryRequiresEnabledAndActiveWindow();
}

TEST(TtcPassManager, AutoEntryTriggersAdcsPointingOnce) {
    OBC::TtcPassManagerTester tester;
    tester.testAutoEntryTriggersAdcsPointingOnce();
}

TEST(TtcPassManager, AdcsTriggerFailureDoesNotBlockTtcEntry) {
    OBC::TtcPassManagerTester tester;
    tester.testAdcsTriggerFailureDoesNotBlockTtcEntry();
}

TEST(TtcPassManager, WindowInactiveExitsTtc) {
    OBC::TtcPassManagerTester tester;
    tester.testWindowInactiveExitsTtc();
}

TEST(TtcPassManager, GpsInvalidExitsTtc) {
    OBC::TtcPassManagerTester tester;
    tester.testGpsInvalidExitsTtc();
}

TEST(TtcPassManager, CommLossTimeoutExitsTtc) {
    OBC::TtcPassManagerTester tester;
    tester.testCommLossTimeoutExitsTtc();
}

TEST(TtcPassManager, CommLossTimeoutUsesElapsedWallclockSeconds) {
    OBC::TtcPassManagerTester tester;
    tester.testCommLossTimeoutUsesElapsedWallclockSeconds();
}

TEST(TtcPassManager, CommLossTimeoutSuppressesReentryUntilWindowChanges) {
    OBC::TtcPassManagerTester tester;
    tester.testCommLossTimeoutSuppressesReentryUntilWindowChanges();
}

TEST(TtcPassManager, ManualTtcStillSubjectToPolicy) {
    OBC::TtcPassManagerTester tester;
    tester.testManualTtcStillSubjectToPolicy();
}

TEST(TtcPassManager, ManualIdleSuppressesReentryUntilWindowChanges) {
    OBC::TtcPassManagerTester tester;
    tester.testManualIdleSuppressesReentryUntilWindowChanges();
}

TEST(TtcPassManager, MidnightGpsSampleStillAllowsEntry) {
    OBC::TtcPassManagerTester tester;
    tester.testMidnightGpsSampleStillAllowsEntry();
}

TEST(TtcPassManager, SafetyOverrideClearsLossTimerWithoutRestoringTtc) {
    OBC::TtcPassManagerTester tester;
    tester.testSafetyOverrideClearsLossTimerWithoutRestoringTtc();
}
