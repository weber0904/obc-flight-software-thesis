#include "CspBridgeTester.hpp"

TEST(CspBridge, InitAndSuccessfulPing) {
    OBC::CspBridgeTester tester;
    tester.testInitAndSuccessfulPing();
}

TEST(CspBridge, SendBeforeInitFails) {
    OBC::CspBridgeTester tester;
    tester.testSendBeforeInitFails();
}

TEST(CspBridge, PingFailureReturnsFalseResult) {
    OBC::CspBridgeTester tester;
    tester.testPingFailureReturnsFalseResult();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
