#include "EpsFdirControllerTester.hpp"

TEST(EpsFdirController, NoEscalationBeforeThreshold) {
    OBC::EpsFdirControllerTester tester;
    tester.testNoEscalationBeforeThreshold();
}

TEST(EpsFdirController, FaultEscalatesOnceAtThreshold) {
    OBC::EpsFdirControllerTester tester;
    tester.testFaultEscalatesOnceAtThreshold();
}

TEST(EpsFdirController, RecoveryClearsFaultOnFirstSuccess) {
    OBC::EpsFdirControllerTester tester;
    tester.testRecoveryClearsFaultOnFirstSuccess();
}

TEST(EpsFdirController, SafeAndHellRecordFaultWithoutModeRequest) {
    OBC::EpsFdirControllerTester tester;
    tester.testSafeAndHellRecordFaultWithoutModeRequest();
}

TEST(EpsFdirController, CountersSaturateAtMax) {
    OBC::EpsFdirControllerTester tester;
    tester.testCountersSaturateAtMax();
}

TEST(EpsFdirController, RelatchReportsSecondFault) {
    OBC::EpsFdirControllerTester tester;
    tester.testRelatchReportsSecondFault();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
