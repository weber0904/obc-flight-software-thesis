#ifndef OBC_COMPONENTS_LINUXWATCHDOGSINK_TEST_UT_LINUXWATCHDOGSINKTESTER_HPP
#define OBC_COMPONENTS_LINUXWATCHDOGSINK_TEST_UT_LINUXWATCHDOGSINKTESTER_HPP

#include "OBC/Components/LinuxWatchdogSink/LinuxWatchdogSink.hpp"
#include "OBC/Components/LinuxWatchdogSink/LinuxWatchdogSinkGTestBase.hpp"

namespace OBC {

class LinuxWatchdogSinkTester final : public LinuxWatchdogSinkGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 32;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    LinuxWatchdogSinkTester();
    ~LinuxWatchdogSinkTester() override;

    void testDisabledRuntimeIgnoresFeeds();
    void testEnabledRuntimeOpensFeedsAndCloses();
    void testOpenFailureLeavesRuntimeClosed();
    void testUnsupportedTimeoutLeavesRuntimeClosed();
    void testKeepaliveFailureSetsError();
    void testGetStatusCommandEmitsEvent();

  private:
    class FakeDevice final : public OBC::ILinuxWatchdogDevice {
      public:
        bool openDevice(const std::string& devicePath, U32 timeoutSec, U32& errorCode) override;
        bool keepalive(U32 code, U32& errorCode) override;
        bool closeDevice(U32& errorCode) override;
        bool isOpen() const override { return this->open; }

        bool failOpen = false;
        bool failKeepalive = false;
        bool open = false;
        U32 supportedTimeout = 0U;
        U32 openCalls = 0U;
        U32 keepaliveCalls = 0U;
        U32 closeCalls = 0U;
        U32 lastCode = 0U;
        std::string lastPath;
        U32 lastTimeout = 0U;
      };

    void initComponents();
    void connectPorts();

  private:
    LinuxWatchdogSink component;
    FakeDevice* fakeDevice;
};

}  // namespace OBC

#endif
