#include "ModeSafetyControllerTester.hpp"

TEST(ModeSafetyController, NoCachedEpsStatusDoesNotTransition) {
    OBC::ModeSafetyControllerTester tester;
    tester.testNoCachedEpsStatusDoesNotTransition();
}

TEST(ModeSafetyController, UnconfiguredRuntimeDoesNotEvaluate) {
    OBC::ModeSafetyControllerTester tester;
    tester.testUnconfiguredRuntimeDoesNotEvaluate();
}

TEST(ModeSafetyController, SafeToHellThresholdIsStrict) {
    OBC::ModeSafetyControllerTester tester;
    tester.testSafeToHellThresholdIsStrict();
}

TEST(ModeSafetyController, HellToSafeThresholdIsStrict) {
    OBC::ModeSafetyControllerTester tester;
    tester.testHellToSafeThresholdIsStrict();
}

TEST(ModeSafetyController, ActiveModesToSafeThresholdIsStrict) {
    OBC::ModeSafetyControllerTester tester;
    tester.testActiveModesToSafeThresholdIsStrict();
}

TEST(ModeSafetyController, PayloadToIdleThresholdAndPriority) {
    OBC::ModeSafetyControllerTester tester;
    tester.testPayloadToIdleThresholdAndPriority();
}

TEST(ModeSafetyController, AlreadyTargetAndHighSocSafeAreNoOps) {
    OBC::ModeSafetyControllerTester tester;
    tester.testAlreadyTargetAndHighSocSafeAreNoOps();
}

TEST(ModeSafetyController, OperatorGuardMatrix) {
    OBC::ModeSafetyControllerTester tester;
    tester.testOperatorGuardMatrix();
}

TEST(ModeSafetyController, OperatorHellToSafeGuardUsesCachedEps) {
    OBC::ModeSafetyControllerTester tester;
    tester.testOperatorHellToSafeGuardUsesCachedEps();
}

TEST(ModeSafetyController, OperatorAdmissionGuardsUseCachedEps) {
    OBC::ModeSafetyControllerTester tester;
    tester.testOperatorAdmissionGuardsUseCachedEps();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
