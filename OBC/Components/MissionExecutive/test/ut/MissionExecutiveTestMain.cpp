#include "MissionExecutiveTester.hpp"

#include <gtest/gtest.h>

TEST(MissionExecutive, HighAngularRateTriggersDetumble) {
    OBC::MissionExecutiveTester tester;
    tester.testHighAngularRateTriggersDetumble();
}

TEST(MissionExecutive, HighRateSuppressesSunSafePointingAtLowBattery) {
    OBC::MissionExecutiveTester tester;
    tester.testHighRateSuppressesSunSafePointingAtLowBattery();
}

TEST(MissionExecutive, LowBatteryTriggersLowPowerAndSunPointing) {
    OBC::MissionExecutiveTester tester;
    tester.testLowBatteryTriggersLowPowerAndSunPointing();
}

TEST(MissionExecutive, PolicyLatchesAfterEntry) {
    OBC::MissionExecutiveTester tester;
    tester.testPolicyLatchesAfterEntry();
}

TEST(MissionExecutive, NominalBatteryDoesNothing) {
    OBC::MissionExecutiveTester tester;
    tester.testNominalBatteryDoesNothing();
}
