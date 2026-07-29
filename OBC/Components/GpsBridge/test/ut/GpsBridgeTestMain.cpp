#include "GpsBridgeTester.hpp"

#include <gtest/gtest.h>

TEST(GpsBridge, GetStatePublishesValidFix) {
    OBC::GpsBridgeTester tester;
    tester.testGetStatePublishesValidFix();
}

TEST(GpsBridge, GetStateReplaysTelemetryWhenValuesUnchanged) {
    OBC::GpsBridgeTester tester;
    tester.testGetStateReplaysTelemetryWhenValuesUnchanged();
}

TEST(GpsBridge, NoFixClearsLatchedFixViaScheduler) {
    OBC::GpsBridgeTester tester;
    tester.testNoFixClearsLatchedFixViaScheduler();
}

TEST(GpsBridge, MalformedSentenceReportsParseError) {
    OBC::GpsBridgeTester tester;
    tester.testMalformedSentenceReportsParseError();
}

TEST(GpsBridge, ReplayModeRejectsWhenUnavailable) {
    OBC::GpsBridgeTester tester;
    tester.testReplayModeRejectsWhenUnavailable();
}

TEST(GpsBridge, LiveUartModeRejectsWhenUnavailable) {
    OBC::GpsBridgeTester tester;
    tester.testLiveUartModeRejectsWhenUnavailable();
}

TEST(GpsBridge, LiveUartModeFallsBackWhenBaudrateEnvOverflows) {
    OBC::GpsBridgeTester tester;
    tester.testLiveUartModeFallsBackWhenBaudrateEnvOverflows();
}

TEST(GpsBridge, LiveUartModePublishesTelemetryWithTestSource) {
    OBC::GpsBridgeTester tester;
    tester.testLiveUartModePublishesTelemetryWithTestSource();
}

TEST(GpsBridge, SourceDepletionReportsSourceError) {
    OBC::GpsBridgeTester tester;
    tester.testSourceDepletionReportsSourceError();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
