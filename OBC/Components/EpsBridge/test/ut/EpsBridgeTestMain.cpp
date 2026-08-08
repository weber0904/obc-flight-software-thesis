#include "EpsBridgeTester.hpp"

#include <gtest/gtest.h>

TEST(EpsBridge, GetStatusPublishesExplicitRefreshTelemetry) {
    OBC::EpsBridgeTester tester;
    tester.testGetStatusPublishesExplicitRefreshTelemetry();
}

TEST(EpsBridge, PduCommandPublishesChangeEventAndExplicitRefresh) {
    OBC::EpsBridgeTester tester;
    tester.testPduCommandPublishesChangeEventAndExplicitRefresh();
}

TEST(EpsBridge, HeaterCommandPublishesExplicitRefreshTelemetry) {
    OBC::EpsBridgeTester tester;
    tester.testHeaterCommandPublishesExplicitRefreshTelemetry();
}

TEST(EpsBridge, ResetPublishesExplicitRefreshTelemetry) {
    OBC::EpsBridgeTester tester;
    tester.testResetPublishesExplicitRefreshTelemetry();
}

TEST(EpsBridge, LowBatteryAndCriticalEvents) {
    OBC::EpsBridgeTester tester;
    tester.testLowBatteryAndCriticalEvents();
}

TEST(EpsBridge, ScheduledPollPublishesContinuousOnly) {
    OBC::EpsBridgeTester tester;
    tester.testScheduledPollPublishesContinuousOnly();
}

TEST(EpsBridge, ScheduledPollPublishesChangeDrivenTelemetryOnStateChange) {
    OBC::EpsBridgeTester tester;
    tester.testScheduledPollPublishesChangeDrivenTelemetryOnStateChange();
}

TEST(EpsBridge, ScheduledPollThrottlesContinuousTelemetry) {
    OBC::EpsBridgeTester tester;
    tester.testScheduledPollThrottlesContinuousTelemetry();
}

TEST(EpsBridge, SchedInUsesAsyncOwnerFlow) {
    OBC::EpsBridgeTester tester;
    tester.testSchedInUsesAsyncOwnerFlow();
}

TEST(EpsBridge, SchedInHonorsConfiguredPollPeriod) {
    OBC::EpsBridgeTester tester;
    tester.testSchedInHonorsConfiguredPollPeriod();
}

TEST(EpsBridge, TimeoutInvalidatesCachedStatusAndPreservesLastTelemetry) {
    OBC::EpsBridgeTester tester;
    tester.testTimeoutInvalidatesCachedStatusAndPreservesLastTelemetry();
}

TEST(EpsBridge, PollHealthTracksConsecutiveFailuresAndRecovery) {
    OBC::EpsBridgeTester tester;
    tester.testPollHealthTracksConsecutiveFailuresAndRecovery();
}

TEST(EpsBridge, RuntimeFetchDoesNotMutatePollHealth) {
    OBC::EpsBridgeTester tester;
    tester.testRuntimeFetchDoesNotMutatePollHealth();
}

TEST(EpsBridge, PollHealthCountersSaturateAtMax) {
    OBC::EpsBridgeTester tester;
    tester.testPollHealthCountersSaturateAtMax();
}
