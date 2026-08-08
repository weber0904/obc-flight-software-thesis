#include "LinuxWatchdogSinkTester.hpp"

namespace OBC {

bool LinuxWatchdogSinkTester::FakeDevice::openDevice(const std::string& devicePath, U32 timeoutSec, U32& errorCode) {
    this->openCalls += 1U;
    this->lastPath = devicePath;
    this->lastTimeout = timeoutSec;
    if (this->failOpen) {
        errorCode = 13U;
        this->open = false;
        return false;
    }
    if (this->supportedTimeout != 0U && timeoutSec != this->supportedTimeout) {
        errorCode = 22U;
        this->open = false;
        return false;
    }
    this->open = true;
    this->keepaliveCalls += 1U;
    this->lastCode = 0U;
    errorCode = 0U;
    return true;
}

bool LinuxWatchdogSinkTester::FakeDevice::keepalive(U32 code, U32& errorCode) {
    this->keepaliveCalls += 1U;
    this->lastCode = code;
    if (this->failKeepalive) {
        errorCode = 5U;
        return false;
    }
    errorCode = 0U;
    return true;
}

bool LinuxWatchdogSinkTester::FakeDevice::closeDevice(U32& errorCode) {
    this->closeCalls += 1U;
    this->open = false;
    errorCode = 0U;
    return true;
}

LinuxWatchdogSinkTester::LinuxWatchdogSinkTester()
    : LinuxWatchdogSinkGTestBase("LinuxWatchdogSinkTester", MAX_HISTORY_SIZE),
      component("LinuxWatchdogSink"),
      fakeDevice(new FakeDevice()) {
    this->initComponents();
    this->connectPorts();
    this->component.setDeviceForTest(std::unique_ptr<OBC::ILinuxWatchdogDevice>(this->fakeDevice));
}

LinuxWatchdogSinkTester::~LinuxWatchdogSinkTester() = default;

void LinuxWatchdogSinkTester::testDisabledRuntimeIgnoresFeeds() {
    ASSERT_TRUE(this->component.configureRuntime(false, "/dev/watchdog0", 15U));
    this->invoke_to_watchdogFeedIn(0, 0x1234U);
    const OBC::LinuxWatchdogRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_FALSE(status.enabled);
    ASSERT_FALSE(status.open);
    ASSERT_EQ(status.feedCount, 0U);
    ASSERT_EQ(this->fakeDevice->openCalls, 0U);
    ASSERT_EQ(this->fakeDevice->keepaliveCalls, 0U);
}

void LinuxWatchdogSinkTester::testEnabledRuntimeOpensFeedsAndCloses() {
    ASSERT_TRUE(this->component.configureRuntime(true, "/dev/watchdog0", 15U));
    OBC::LinuxWatchdogRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.enabled);
    ASSERT_TRUE(status.open);
    ASSERT_EQ(this->fakeDevice->openCalls, 1U);
    ASSERT_EQ(this->fakeDevice->lastTimeout, 15U);
    ASSERT_EQ(this->fakeDevice->keepaliveCalls, 1U);

    this->invoke_to_watchdogFeedIn(0, 0xCAFEU);
    status = this->component.getStatusForRuntime();
    ASSERT_EQ(status.feedCount, 1U);
    ASSERT_EQ(status.lastFeedCode, 0xCAFEU);
    ASSERT_EQ(this->fakeDevice->keepaliveCalls, 2U);

    ASSERT_TRUE(this->component.shutdownForRuntime());
    status = this->component.getStatusForRuntime();
    ASSERT_FALSE(status.open);
    ASSERT_EQ(this->fakeDevice->closeCalls, 1U);
}

void LinuxWatchdogSinkTester::testOpenFailureLeavesRuntimeClosed() {
    this->fakeDevice->failOpen = true;
    ASSERT_FALSE(this->component.configureRuntime(true, "/dev/watchdog0", 15U));
    const OBC::LinuxWatchdogRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.enabled);
    ASSERT_FALSE(status.open);
    ASSERT_EQ(status.lastError, 13U);
}

void LinuxWatchdogSinkTester::testUnsupportedTimeoutLeavesRuntimeClosed() {
    this->fakeDevice->supportedTimeout = 15U;
    ASSERT_FALSE(this->component.configureRuntime(true, "/dev/watchdog0", 30U));
    const OBC::LinuxWatchdogRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.enabled);
    ASSERT_FALSE(status.open);
    ASSERT_EQ(status.lastError, 22U);
    ASSERT_EQ(this->fakeDevice->lastTimeout, 30U);
}

void LinuxWatchdogSinkTester::testKeepaliveFailureSetsError() {
    ASSERT_TRUE(this->component.configureRuntime(true, "/dev/watchdog0", 15U));
    this->fakeDevice->failKeepalive = true;
    this->invoke_to_watchdogFeedIn(0, 0x55AAU);
    const OBC::LinuxWatchdogRuntimeStatus status = this->component.getStatusForRuntime();
    ASSERT_TRUE(status.open);
    ASSERT_EQ(status.lastError, 5U);
    ASSERT_EQ(status.feedCount, 0U);
}

void LinuxWatchdogSinkTester::testGetStatusCommandEmitsEvent() {
    ASSERT_TRUE(this->component.configureRuntime(true, "/dev/watchdog0", 15U));

    this->clearHistory();
    this->sendCmd_GET_HW_WATCHDOG_STATUS(TEST_INSTANCE_ID, 0U);

    ASSERT_CMD_RESPONSE_SIZE(1);
    ASSERT_CMD_RESPONSE(0, this->component.OPCODE_GET_HW_WATCHDOG_STATUS, 0U, Fw::CmdResponse::OK);
    ASSERT_EVENTS_HW_WATCHDOG_STATUS_SIZE(1);
    ASSERT_EVENTS_HW_WATCHDOG_STATUS(0, true, true, 15U, 0U, 0U);
}

}  // namespace OBC
