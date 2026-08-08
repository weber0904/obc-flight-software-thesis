#include "AdcsFdirControllerTester.hpp"

TEST(AdcsFdirController, TransportRetriesBeforeLatch) {
    OBC::AdcsFdirControllerTester tester;
    tester.testTransportRetriesBeforeLatch();
}

TEST(AdcsFdirController, FreshnessFaultLatchesAtThreshold) {
    OBC::AdcsFdirControllerTester tester;
    tester.testFreshnessFaultLatchesAtThreshold();
}

TEST(AdcsFdirController, HealthyScheduledCycleClearsFault) {
    OBC::AdcsFdirControllerTester tester;
    tester.testHealthyScheduledCycleClearsFault();
}

TEST(AdcsFdirController, LatchedFaultDoesNotSwitchSourceBeforeHealthyClear) {
    OBC::AdcsFdirControllerTester tester;
    tester.testLatchedFaultDoesNotSwitchSourceBeforeHealthyClear();
}

TEST(AdcsFdirController, SchedEmitsWatchdogBeat) {
    OBC::AdcsFdirControllerTester tester;
    tester.testSchedEmitsWatchdogBeat();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
