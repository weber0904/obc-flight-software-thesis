#include "AdcsBridgeTester.hpp"

#include <gtest/gtest.h>

TEST(AdcsBridge, GetAttitudePublishesExplicitRefreshTelemetry) {
    OBC::AdcsBridgeTester tester;
    tester.testGetAttitudePublishesExplicitRefreshTelemetry();
}

TEST(AdcsBridge, GetAttitudeRepublishesExplicitRefreshWhenValuesUnchanged) {
    OBC::AdcsBridgeTester tester;
    tester.testGetAttitudeRepublishesExplicitRefreshWhenValuesUnchanged();
}

TEST(AdcsBridge, SetModeCanTriggerDetumbleCompletion) {
    OBC::AdcsBridgeTester tester;
    tester.testSetModeCanTriggerDetumbleCompletion();
}

TEST(AdcsBridge, PointingAcquiredAfterTargetUpdate) {
    OBC::AdcsBridgeTester tester;
    tester.testPointingAcquiredAfterTargetUpdate();
}

TEST(AdcsBridge, ScheduledPollPublishesContinuousOnly) {
    OBC::AdcsBridgeTester tester;
    tester.testScheduledPollPublishesContinuousOnly();
}

TEST(AdcsBridge, ScheduledPollPublishesChangeDrivenTelemetryOnModeChange) {
    OBC::AdcsBridgeTester tester;
    tester.testScheduledPollPublishesChangeDrivenTelemetryOnModeChange();
}

TEST(AdcsBridge, ScheduledPollThrottlesContinuousTelemetry) {
    OBC::AdcsBridgeTester tester;
    tester.testScheduledPollThrottlesContinuousTelemetry();
}

TEST(AdcsBridge, SchedInUsesAsyncOwnerFlow) {
    OBC::AdcsBridgeTester tester;
    tester.testSchedInUsesAsyncOwnerFlow();
}

TEST(AdcsBridge, SchedInHonorsConfiguredPollPeriod) {
    OBC::AdcsBridgeTester tester;
    tester.testSchedInHonorsConfiguredPollPeriod();
}

TEST(AdcsBridge, InvalidSensorReplyPreservesLastTelemetry) {
    OBC::AdcsBridgeTester tester;
    tester.testInvalidSensorReplyPreservesLastTelemetry();
}

TEST(AdcsBridge, TimeoutPreservesLastTelemetry) {
    OBC::AdcsBridgeTester tester;
    tester.testTimeoutPreservesLastTelemetry();
}

TEST(AdcsBridge, CommandFailureDoesNotMutateScheduledPollHealth) {
    OBC::AdcsBridgeTester tester;
    tester.testCommandFailureDoesNotMutateScheduledPollHealth();
}

TEST(AdcsBridge, RecoveryResetUpdatesCachedState) {
    OBC::AdcsBridgeTester tester;
    tester.testRecoveryResetUpdatesCachedState();
}

TEST(AdcsBridge, RecoveryResetFailureDoesNotMutateScheduledPollHealth) {
    OBC::AdcsBridgeTester tester;
    tester.testRecoveryResetFailureDoesNotMutateScheduledPollHealth();
}
