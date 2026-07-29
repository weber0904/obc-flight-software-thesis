#include "LinuxWatchdogSinkTester.hpp"

TEST(LinuxWatchdogSink, DisabledRuntimeIgnoresFeeds) {
    OBC::LinuxWatchdogSinkTester tester;
    tester.testDisabledRuntimeIgnoresFeeds();
}

TEST(LinuxWatchdogSink, EnabledRuntimeOpensFeedsAndCloses) {
    OBC::LinuxWatchdogSinkTester tester;
    tester.testEnabledRuntimeOpensFeedsAndCloses();
}

TEST(LinuxWatchdogSink, OpenFailureLeavesRuntimeClosed) {
    OBC::LinuxWatchdogSinkTester tester;
    tester.testOpenFailureLeavesRuntimeClosed();
}

TEST(LinuxWatchdogSink, UnsupportedTimeoutLeavesRuntimeClosed) {
    OBC::LinuxWatchdogSinkTester tester;
    tester.testUnsupportedTimeoutLeavesRuntimeClosed();
}

TEST(LinuxWatchdogSink, KeepaliveFailureSetsError) {
    OBC::LinuxWatchdogSinkTester tester;
    tester.testKeepaliveFailureSetsError();
}

TEST(LinuxWatchdogSink, GetStatusCommandEmitsEvent) {
    OBC::LinuxWatchdogSinkTester tester;
    tester.testGetStatusCommandEmitsEvent();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
