#ifndef OBC_COMPONENTS_LINUXWATCHDOGSINK_LINUXWATCHDOGSINK_HPP
#define OBC_COMPONENTS_LINUXWATCHDOGSINK_LINUXWATCHDOGSINK_HPP

#include <memory>
#include <mutex>
#include <string>

#include "OBC/Components/LinuxWatchdogSink/LinuxWatchdogRuntime.hpp"
#include "OBC/Components/LinuxWatchdogSink/LinuxWatchdogSinkComponentAc.hpp"

namespace OBC {

class ILinuxWatchdogDevice {
  public:
    virtual ~ILinuxWatchdogDevice() = default;
    virtual bool openDevice(const std::string& devicePath, U32 timeoutSec, U32& errorCode) = 0;
    virtual bool keepalive(U32 code, U32& errorCode) = 0;
    virtual bool closeDevice(U32& errorCode) = 0;
    virtual bool isOpen() const = 0;
};

class LinuxWatchdogSink final : public LinuxWatchdogSinkComponentBase {
  public:
    explicit LinuxWatchdogSink(const char* compName);

    ~LinuxWatchdogSink() override;

    bool configureRuntime(bool enabled, const std::string& devicePath, U32 timeoutSec);

    bool shutdownForRuntime();

    OBC::LinuxWatchdogRuntimeStatus getStatusForRuntime() const;

#ifdef BUILD_UT
    void setDeviceForTest(std::unique_ptr<OBC::ILinuxWatchdogDevice> device);
#endif

  private:
    void GET_HW_WATCHDOG_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;
    void watchdogFeedIn_handler(FwIndexType portNum, U32 code) override;

    void publishTelemetry_(const OBC::LinuxWatchdogRuntimeStatus& status);
    void ensureDevice_();
    bool shutdownLocked_(bool emitEvent, bool emitTelemetry);
    bool configureLocked_(bool enabled, const std::string& devicePath, U32 timeoutSec);

  private:
    std::unique_ptr<OBC::ILinuxWatchdogDevice> m_device;
    OBC::LinuxWatchdogRuntimeStatus m_status;
    mutable std::mutex m_mutex;
};

}  // namespace OBC

#endif
