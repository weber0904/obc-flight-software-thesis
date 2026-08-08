#include "ModeManagerTester.hpp"

TEST(ModeManager, ModeSetPublishesTelemetry) {
    OBC::ModeManagerTester tester;
    tester.testModeSetPublishesTelemetry();
}

TEST(ModeManager, ModeSetRejectedPreservesMode) {
    OBC::ModeManagerTester tester;
    tester.testModeSetRejectedPreservesMode();
}

TEST(ModeManager, SameModeNoOpDoesNotEmitModeChange) {
    OBC::ModeManagerTester tester;
    tester.testSameModeNoOpDoesNotEmitModeChange();
}

TEST(ModeManager, GuardUnconfiguredReturnsExecutionError) {
    OBC::ModeManagerTester tester;
    tester.testGuardUnconfiguredReturnsExecutionError();
}

TEST(ModeManager, AllOperatorPairsUseGuardedPath) {
    OBC::ModeManagerTester tester;
    tester.testAllOperatorPairsUseGuardedPath();
}

TEST(ModeManager, ModeGetRepublishesStateWhenUnchanged) {
    OBC::ModeManagerTester tester;
    tester.testModeGetRepublishesStateWhenUnchanged();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
