#include "OBC/Components/LinuxWatchdogSink/LinuxWatchdogSink.hpp"

#include <cerrno>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#if defined(__linux__)
#include <linux/watchdog.h>
#include <sys/ioctl.h>
#endif

namespace OBC {

namespace {

class PosixLinuxWatchdogDevice final : public OBC::ILinuxWatchdogDevice {
  public:
    PosixLinuxWatchdogDevice() : m_fd(-1) {}

    ~PosixLinuxWatchdogDevice() override {
        U32 ignored = 0U;
        static_cast<void>(this->closeDevice(ignored));
    }

    bool openDevice(const std::string& devicePath, U32 timeoutSec, U32& errorCode) override {
        errorCode = 0U;
        if (devicePath.empty() || timeoutSec == 0U) {
            errorCode = static_cast<U32>(EINVAL);
            return false;
        }

        const int fd = ::open(devicePath.c_str(), O_WRONLY | O_CLOEXEC);
        if (fd < 0) {
            errorCode = static_cast<U32>(errno);
            return false;
        }
        this->m_fd = fd;
        if (!this->configureTimeout_(timeoutSec, errorCode)) {
            U32 closeError = 0U;
            static_cast<void>(this->closeDevice(closeError));
            if (errorCode == 0U) {
                errorCode = closeError == 0U ? static_cast<U32>(EIO) : closeError;
            }
            return false;
        }
        if (!this->keepalive(0U, errorCode)) {
            U32 closeError = 0U;
            static_cast<void>(this->closeDevice(closeError));
            if (errorCode == 0U) {
                errorCode = closeError == 0U ? static_cast<U32>(EIO) : closeError;
            }
            return false;
        }
        return true;
    }

    bool keepalive(U32, U32& errorCode) override {
        errorCode = 0U;
        if (this->m_fd < 0) {
            errorCode = static_cast<U32>(EBADF);
            return false;
        }

        const char kick = '1';
        const ssize_t written = ::write(this->m_fd, &kick, 1);
        if (written != 1) {
            errorCode = static_cast<U32>(errno == 0 ? EIO : errno);
            return false;
        }
        return true;
    }

    bool closeDevice(U32& errorCode) override {
        errorCode = 0U;
        if (this->m_fd < 0) {
            return true;
        }
        const int fd = this->m_fd;
        this->m_fd = -1;
        if (::close(fd) != 0) {
            errorCode = static_cast<U32>(errno == 0 ? EIO : errno);
            return false;
        }
        return true;
    }

    bool isOpen() const override { return this->m_fd >= 0; }

  private:
    bool configureTimeout_(U32 timeoutSec, U32& errorCode) {
        errorCode = 0U;
#if defined(__linux__)
        int requestedTimeout = static_cast<int>(timeoutSec);
        if (::ioctl(this->m_fd, WDIOC_SETTIMEOUT, &requestedTimeout) == 0) {
            if (requestedTimeout <= 0 || static_cast<U32>(requestedTimeout) != timeoutSec) {
                errorCode = static_cast<U32>(EIO);
                return false;
            }
            return true;
        }

        const U32 setTimeoutError = static_cast<U32>(errno == 0 ? EIO : errno);
        int currentTimeout = 0;
        if (::ioctl(this->m_fd, WDIOC_GETTIMEOUT, &currentTimeout) == 0) {
            if (currentTimeout > 0 && static_cast<U32>(currentTimeout) == timeoutSec) {
                errorCode = 0U;
                return true;
            }
        }
        errorCode = setTimeoutError;
        return false;
#else
        static_cast<void>(timeoutSec);
        return true;
#endif
    }

    int m_fd;
};

}  // namespace

LinuxWatchdogSink::LinuxWatchdogSink(const char* compName) : LinuxWatchdogSinkComponentBase(compName), m_device(), m_status() {}

LinuxWatchdogSink::~LinuxWatchdogSink() {
    static_cast<void>(this->shutdownForRuntime());
}

bool LinuxWatchdogSink::configureRuntime(bool enabled, const std::string& devicePath, U32 timeoutSec) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->configureLocked_(enabled, devicePath, timeoutSec);
}

bool LinuxWatchdogSink::shutdownForRuntime() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    // Runtime teardown can happen during process exit after time/event services
    // have already started unwinding, so avoid any port-driven telemetry/event
    // emission on this path.
    return this->shutdownLocked_(false, false);
}

OBC::LinuxWatchdogRuntimeStatus LinuxWatchdogSink::getStatusForRuntime() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_status;
}

#ifdef BUILD_UT
void LinuxWatchdogSink::setDeviceForTest(std::unique_ptr<OBC::ILinuxWatchdogDevice> device) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_device = std::move(device);
}
#endif

void LinuxWatchdogSink::GET_HW_WATCHDOG_STATUS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    const OBC::LinuxWatchdogRuntimeStatus status = this->getStatusForRuntime();
    this->publishTelemetry_(status);
    this->log_ACTIVITY_HI_HW_WATCHDOG_STATUS(
        status.enabled, status.open, status.timeoutSec, status.feedCount, status.lastError);
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
}

void LinuxWatchdogSink::watchdogFeedIn_handler(FwIndexType portNum, U32 code) {
    static_cast<void>(portNum);

    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->m_status.enabled || this->m_device == nullptr || !this->m_device->isOpen()) {
        return;
    }

    U32 errorCode = 0U;
    if (!this->m_device->keepalive(code, errorCode)) {
        this->m_status.lastError = errorCode;
        this->m_status.open = this->m_device->isOpen();
        this->publishTelemetry_(this->m_status);
        this->log_WARNING_HI_HW_WATCHDOG_KEEPALIVE_FAILED(errorCode);
        return;
    }

    this->m_status.feedCount += 1U;
    this->m_status.lastFeedCode = code;
    this->m_status.lastError = 0U;
}

void LinuxWatchdogSink::publishTelemetry_(const OBC::LinuxWatchdogRuntimeStatus& status) {
    this->tlmWrite_HW_WATCHDOG_ENABLED(status.enabled);
    this->tlmWrite_HW_WATCHDOG_DEVICE_OPEN(status.open);
    this->tlmWrite_HW_WATCHDOG_TIMEOUT_SEC(status.timeoutSec);
    this->tlmWrite_HW_WATCHDOG_FEED_COUNT(status.feedCount);
    this->tlmWrite_HW_WATCHDOG_LAST_FEED_CODE(status.lastFeedCode);
    this->tlmWrite_HW_WATCHDOG_LAST_ERROR(status.lastError);
}

void LinuxWatchdogSink::ensureDevice_() {
    if (this->m_device == nullptr) {
        this->m_device = std::unique_ptr<OBC::ILinuxWatchdogDevice>(new PosixLinuxWatchdogDevice());
    }
}

bool LinuxWatchdogSink::shutdownLocked_(bool emitEvent, bool emitTelemetry) {
    U32 errorCode = 0U;
    bool ok = true;
    if (this->m_device != nullptr && this->m_device->isOpen()) {
        ok = this->m_device->closeDevice(errorCode);
        this->m_status.lastError = ok ? 0U : errorCode;
    }
    this->m_status.open = false;
    if (emitTelemetry) {
        this->publishTelemetry_(this->m_status);
    }
    if (emitEvent && ok) {
        this->log_ACTIVITY_HI_HW_WATCHDOG_CLOSED();
    }
    return ok;
}

bool LinuxWatchdogSink::configureLocked_(bool enabled, const std::string& devicePath, U32 timeoutSec) {
    static_cast<void>(this->shutdownLocked_(false, false));
    this->m_status.enabled = enabled;
    this->m_status.devicePath = devicePath;
    this->m_status.timeoutSec = timeoutSec;
    this->m_status.feedCount = 0U;
    this->m_status.lastFeedCode = 0U;
    this->m_status.lastError = 0U;
    this->publishTelemetry_(this->m_status);
    this->log_ACTIVITY_HI_HW_WATCHDOG_CONFIG_UPDATED(enabled, timeoutSec);

    if (!enabled) {
        return true;
    }

    this->ensureDevice_();
    U32 errorCode = 0U;
    if (!this->m_device->openDevice(devicePath, timeoutSec, errorCode)) {
        this->m_status.open = false;
        this->m_status.lastError = errorCode;
        this->publishTelemetry_(this->m_status);
        this->log_WARNING_HI_HW_WATCHDOG_OPEN_FAILED(errorCode);
        return false;
    }

    this->m_status.open = true;
    this->m_status.lastError = 0U;
    this->publishTelemetry_(this->m_status);
    this->log_ACTIVITY_HI_HW_WATCHDOG_OPENED(timeoutSec);
    return true;
}

}  // namespace OBC
