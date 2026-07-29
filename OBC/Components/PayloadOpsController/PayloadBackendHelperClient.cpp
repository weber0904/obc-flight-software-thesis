#include "OBC/Components/PayloadOpsController/PayloadOpsRuntime.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <signal.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "OBC/Components/PayloadOpsController/PayloadBackendHelperProtocol.hpp"
#include "OBC/Components/PayloadOpsController/PayloadBackendHelperProtocolUtils.hpp"

namespace OBC {

namespace {

constexpr char HELPER_ENV_VAR[] = "OBC_PAYLOAD_HELPER_BIN";
constexpr char HELPER_BINARY_NAME[] = "payload_camera_backend_helper";

constexpr U32 DETAIL_HELPER_PATH_UNAVAILABLE = 400U;
constexpr U32 DETAIL_HELPER_PIPE_FAILED = 401U;
constexpr U32 DETAIL_HELPER_FORK_FAILED = 402U;
constexpr U32 DETAIL_HELPER_EXEC_FAILED = 403U;
constexpr U32 DETAIL_HELPER_WRITE_FAILED = 404U;
constexpr U32 DETAIL_HELPER_READ_FAILED = 405U;
constexpr U32 DETAIL_HELPER_TIMEOUT = 406U;
constexpr U32 DETAIL_HELPER_ABORTED = 407U;
constexpr U32 DETAIL_HELPER_PROTOCOL_MISMATCH = 408U;
constexpr U32 DETAIL_HELPER_UNAVAILABLE = 409U;
constexpr U32 DETAIL_HELPER_SHUTDOWN_TIMEOUT = 410U;

std::string parentDirectoryOf_(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        return ".";
    }
    if (slash == 0U) {
        return "/";
    }
    return path.substr(0U, slash);
}

std::string currentExecutablePath_() {
#ifdef __APPLE__
    uint32_t size = 0U;
    if (_NSGetExecutablePath(nullptr, &size) != -1) {
        return std::string();
    }
    std::string path(size + 1U, '\0');
    if (_NSGetExecutablePath(&path[0], &size) != 0) {
        return std::string();
    }
    path.resize(strnlen(path.c_str(), path.size()));
    return path;
#else
    char buffer[1024] = {};
    const ssize_t length = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length <= 0) {
        return std::string();
    }
    buffer[length] = '\0';
    return std::string(buffer);
#endif
}

std::string resolveHelperBinaryPath_() {
    const char* envPath = std::getenv(HELPER_ENV_VAR);
    if (envPath != nullptr && envPath[0] != '\0') {
        return std::string(envPath);
    }

    const std::string executable = currentExecutablePath_();
    if (executable.empty()) {
        return std::string(HELPER_BINARY_NAME);
    }
    return parentDirectoryOf_(executable) + "/" + HELPER_BINARY_NAME;
}

bool isExecutableFile_(const std::string& path) {
    return !path.empty() && ::access(path.c_str(), X_OK) == 0;
}

bool configureNoSigPipe_(int fd) {
#ifdef __APPLE__
    const int one = 1;
    return ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one)) == 0;
#else
    static_cast<void>(fd);
    return true;
#endif
}

bool waitForFd_(int fd, short events, const std::chrono::steady_clock::time_point deadline) {
    constexpr long long POLL_SLICE_MS = 50LL;
    while (true) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
            return false;
        }
        const auto remainingMs =
            std::max<long long>(1LL,
                                std::min<long long>(POLL_SLICE_MS,
                                                    std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now)
                                                        .count()));
        struct pollfd pfd = {};
        pfd.fd = fd;
        pfd.events = events;
        const int rc = ::poll(&pfd, 1, static_cast<int>(remainingMs));
        if (rc > 0) {
            return (pfd.revents & events) != 0;
        }
        if (rc == 0) {
            continue;
        }
        if (errno != EINTR) {
            return false;
        }
    }
}

class PiCameraBackendHelperClient final : public OBC::IPiCameraDriver {
  public:
    PiCameraBackendHelperClient()
        : m_transactionMutex(),
          m_stateMutex(),
          m_pid(-1),
          m_reapPendingPid(-1),
          m_requestFd(-1),
          m_replyFd(-1),
          m_abortRequested(false),
          m_cachedCapabilities(),
          m_capabilitiesValid(false) {}

    ~PiCameraBackendHelperClient() override {
        this->abort();
    }

    const char* getName() const override { return "payload-helper"; }

    bool isAvailable() const override {
        if (this->m_pid.load() > 0) {
            return true;
        }
        return isExecutableFile_(resolveHelperBinaryPath_());
    }

    OBC::PayloadCapabilities getCapabilities() const override {
        std::lock_guard<std::mutex> transactionLock(this->m_transactionMutex);
        {
            std::lock_guard<std::mutex> stateLock(this->m_stateMutex);
            if (this->m_capabilitiesValid) {
                return this->m_cachedCapabilities;
            }
        }

        OBC::PayloadHelperProtocol::EmptyRequest request = {};
        request.header.magic = OBC::PayloadHelperProtocol::MAGIC;
        request.header.version = OBC::PayloadHelperProtocol::VERSION;
        request.header.opCode = static_cast<U16>(OBC::PayloadHelperProtocol::OpCode::GET_CAPABILITIES);

        OBC::PayloadHelperProtocol::Reply reply = {};
        U32 detailCode = 0U;
        if (this->transactLocked_(request, sizeof(request), reply, 1000U, nullptr, detailCode) != Fw::CmdResponse::OK) {
            OBC::PayloadCapabilities unavailable = {};
            unavailable.backendName = "helper-unavailable";
            unavailable.cameraModel = "unknown";
            return unavailable;
        }

        {
            std::lock_guard<std::mutex> stateLock(this->m_stateMutex);
            this->m_cachedCapabilities = OBC::PayloadHelperProtocolUtils::decodeCapabilities(reply.capabilities);
            this->m_capabilitiesValid = true;
        }
        return this->m_cachedCapabilities;
    }

    Fw::CmdResponse prepare(const OBC::PayloadReadyKind readyKind,
                            const OBC::PayloadCameraSettings& settings,
                            U32 initTimeoutMs,
                            const std::atomic<bool>& cancelRequested,
                            U32& detailCode) override {
        static_cast<void>(readyKind);
        std::lock_guard<std::mutex> transactionLock(this->m_transactionMutex);
        this->m_abortRequested.store(false);
        OBC::PayloadHelperProtocol::PrepareRequest request = {};
        request.header.magic = OBC::PayloadHelperProtocol::MAGIC;
        request.header.version = OBC::PayloadHelperProtocol::VERSION;
        request.header.opCode = static_cast<U16>(OBC::PayloadHelperProtocol::OpCode::PREPARE);
        request.settings = OBC::PayloadHelperProtocolUtils::encodeSettings(settings);
        request.initTimeoutMs = initTimeoutMs;

        OBC::PayloadHelperProtocol::Reply reply = {};
        return this->transactLocked_(request, sizeof(request), reply, initTimeoutMs, &cancelRequested, detailCode);
    }

    Fw::CmdResponse captureStill(const OBC::PayloadCaptureRequest& request,
                                 const OBC::PayloadCameraSettings& settings,
                                 U32 captureTimeoutMs,
                                 const std::atomic<bool>& cancelRequested,
                                 OBC::PayloadCaptureMetadata& metadata,
                                 U32& detailCode) override {
        std::lock_guard<std::mutex> transactionLock(this->m_transactionMutex);
        this->m_abortRequested.store(false);
        OBC::PayloadHelperProtocol::CaptureRequestWire wire = {};
        wire.header.magic = OBC::PayloadHelperProtocol::MAGIC;
        wire.header.version = OBC::PayloadHelperProtocol::VERSION;
        wire.header.opCode = static_cast<U16>(OBC::PayloadHelperProtocol::OpCode::CAPTURE);
        wire.settings = OBC::PayloadHelperProtocolUtils::encodeSettings(settings);
        wire.captureTimeoutMs = captureTimeoutMs;
        wire.captureId = request.captureId;
        wire.captureIndex = request.captureIndex;
        wire.bootCount = request.bootCount;
        wire.requestedMask = request.requestedMask;
        wire.appliedMask = request.appliedMask;
        OBC::PayloadHelperProtocolUtils::copyBoundedString(wire.tag, request.tag);
        OBC::PayloadHelperProtocolUtils::copyBoundedString(wire.rawOutputPath, request.rawOutputPath);
        OBC::PayloadHelperProtocolUtils::copyBoundedString(wire.previewOutputPath, request.previewOutputPath);
        OBC::PayloadHelperProtocolUtils::copyBoundedString(wire.rawRelativePath, request.rawRelativePath);
        OBC::PayloadHelperProtocolUtils::copyBoundedString(wire.previewRelativePath, request.previewRelativePath);

        OBC::PayloadHelperProtocol::Reply reply = {};
        const Fw::CmdResponse response =
            this->transactLocked_(wire, sizeof(wire), reply, captureTimeoutMs, &cancelRequested, detailCode);
        if (response == Fw::CmdResponse::OK) {
            metadata = OBC::PayloadHelperProtocolUtils::decodeMetadata(reply.metadata);
        }
        return response;
    }

    Fw::CmdResponse shutdown(U32& detailCode) override {
        std::lock_guard<std::mutex> transactionLock(this->m_transactionMutex);
        if (this->m_pid.load() <= 0) {
            detailCode = 0U;
            return Fw::CmdResponse::OK;
        }

        OBC::PayloadHelperProtocol::ShutdownRequest request = {};
        request.header.magic = OBC::PayloadHelperProtocol::MAGIC;
        request.header.version = OBC::PayloadHelperProtocol::VERSION;
        request.header.opCode = static_cast<U16>(OBC::PayloadHelperProtocol::OpCode::SHUTDOWN);

        OBC::PayloadHelperProtocol::Reply reply = {};
        const Fw::CmdResponse response = this->transactLocked_(request, sizeof(request), reply, 1000U, nullptr, detailCode);
        {
            std::lock_guard<std::mutex> stateLock(this->m_stateMutex);
            this->killHelperLocked_(SIGTERM);
        }
        if (response != Fw::CmdResponse::OK && detailCode == DETAIL_HELPER_TIMEOUT) {
            detailCode = DETAIL_HELPER_SHUTDOWN_TIMEOUT;
        }
        return response;
    }

    Fw::CmdResponse readSensorRegister(U32 address, U32 timeoutMs, U32& value, U32& detailCode) override {
        std::lock_guard<std::mutex> transactionLock(this->m_transactionMutex);
        this->m_abortRequested.store(false);
        OBC::PayloadHelperProtocol::RegisterReadRequest request = {};
        request.header.magic = OBC::PayloadHelperProtocol::MAGIC;
        request.header.version = OBC::PayloadHelperProtocol::VERSION;
        request.header.opCode = static_cast<U16>(OBC::PayloadHelperProtocol::OpCode::READ_REGISTER);
        request.address = address;
        request.timeoutMs = timeoutMs;

        OBC::PayloadHelperProtocol::Reply reply = {};
        const Fw::CmdResponse response = this->transactLocked_(request, sizeof(request), reply, timeoutMs, nullptr, detailCode);
        if (response == Fw::CmdResponse::OK) {
            value = reply.value;
        }
        return response;
    }

    Fw::CmdResponse writeSensorRegister(U32 address,
                                        U32 value,
                                        bool verifyReadback,
                                        U32 timeoutMs,
                                        U32& readbackValue,
                                        U32& detailCode) override {
        std::lock_guard<std::mutex> transactionLock(this->m_transactionMutex);
        this->m_abortRequested.store(false);
        OBC::PayloadHelperProtocol::RegisterWriteRequest request = {};
        request.header.magic = OBC::PayloadHelperProtocol::MAGIC;
        request.header.version = OBC::PayloadHelperProtocol::VERSION;
        request.header.opCode = static_cast<U16>(OBC::PayloadHelperProtocol::OpCode::WRITE_REGISTER);
        request.address = address;
        request.value = value;
        request.timeoutMs = timeoutMs;
        request.verifyReadback = verifyReadback ? 1U : 0U;

        OBC::PayloadHelperProtocol::Reply reply = {};
        const Fw::CmdResponse response = this->transactLocked_(request, sizeof(request), reply, timeoutMs, nullptr, detailCode);
        if (response == Fw::CmdResponse::OK) {
            readbackValue = reply.readbackValue;
        }
        return response;
    }

    void abort() override {
        this->m_abortRequested.store(true);
        std::lock_guard<std::mutex> stateLock(this->m_stateMutex);
        this->killHelperLocked_(SIGKILL);
    }

  private:
    template <typename TRequest>
    Fw::CmdResponse transactLocked_(const TRequest& request,
                                    std::size_t requestSize,
                                    OBC::PayloadHelperProtocol::Reply& reply,
                                    U32 timeoutMs,
                                    const std::atomic<bool>* cancelRequested,
                                    U32& detailCode) const {
        detailCode = 0U;
        if (!this->ensureHelperLocked_(detailCode)) {
            return Fw::CmdResponse::EXECUTION_ERROR;
        }

        const auto deadline =
            std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs == 0U ? 1000U : timeoutMs);
        if (!this->writeAllLocked_(reinterpret_cast<const U8*>(&request), requestSize, deadline, cancelRequested, detailCode)) {
            std::lock_guard<std::mutex> stateLock(this->m_stateMutex);
            this->killHelperLocked_(SIGKILL);
            return detailCode == DETAIL_HELPER_PROTOCOL_MISMATCH ? Fw::CmdResponse::VALIDATION_ERROR
                                                                 : Fw::CmdResponse::EXECUTION_ERROR;
        }

        if (!this->readAllLocked_(reinterpret_cast<U8*>(&reply), sizeof(reply), deadline, cancelRequested, detailCode)) {
            std::lock_guard<std::mutex> stateLock(this->m_stateMutex);
            this->killHelperLocked_(SIGKILL);
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        if (reply.header.magic != OBC::PayloadHelperProtocol::MAGIC ||
            reply.header.version != OBC::PayloadHelperProtocol::VERSION) {
            detailCode = DETAIL_HELPER_PROTOCOL_MISMATCH;
            std::lock_guard<std::mutex> stateLock(this->m_stateMutex);
            this->killHelperLocked_(SIGKILL);
            return Fw::CmdResponse::EXECUTION_ERROR;
        }
        detailCode = reply.detailCode;
        return Fw::CmdResponse(static_cast<Fw::CmdResponse::T>(reply.response));
    }

    bool ensureHelperLocked_(U32& detailCode) const {
        std::lock_guard<std::mutex> stateLock(this->m_stateMutex);
        this->reapPendingHelperLocked_();
        if (this->m_reapPendingPid.load() > 0) {
            detailCode = DETAIL_HELPER_UNAVAILABLE;
            return false;
        }
        if (this->m_pid.load() > 0) {
            return true;
        }

        const std::string helperPath = resolveHelperBinaryPath_();
        if (!isExecutableFile_(helperPath)) {
            detailCode = DETAIL_HELPER_PATH_UNAVAILABLE;
            return false;
        }

        int requestPipe[2] = {-1, -1};
        int replyPipe[2] = {-1, -1};
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, requestPipe) != 0 ||
            ::socketpair(AF_UNIX, SOCK_STREAM, 0, replyPipe) != 0 ||
            !configureNoSigPipe_(requestPipe[0]) || !configureNoSigPipe_(requestPipe[1]) ||
            !configureNoSigPipe_(replyPipe[0]) || !configureNoSigPipe_(replyPipe[1])) {
            if (requestPipe[0] >= 0) {
                ::close(requestPipe[0]);
                ::close(requestPipe[1]);
            }
            if (replyPipe[0] >= 0) {
                ::close(replyPipe[0]);
                ::close(replyPipe[1]);
            }
            detailCode = DETAIL_HELPER_PIPE_FAILED;
            return false;
        }

        const pid_t pid = ::fork();
        if (pid < 0) {
            ::close(requestPipe[0]);
            ::close(requestPipe[1]);
            ::close(replyPipe[0]);
            ::close(replyPipe[1]);
            detailCode = DETAIL_HELPER_FORK_FAILED;
            return false;
        }

        if (pid == 0) {
            ::dup2(requestPipe[0], STDIN_FILENO);
            ::dup2(replyPipe[1], STDOUT_FILENO);
            ::close(requestPipe[0]);
            ::close(requestPipe[1]);
            ::close(replyPipe[0]);
            ::close(replyPipe[1]);
            ::execl(helperPath.c_str(), helperPath.c_str(), static_cast<char*>(nullptr));
            _exit(127);
        }

        ::close(requestPipe[0]);
        ::close(replyPipe[1]);
        this->m_pid.store(pid);
        this->m_requestFd.store(requestPipe[1]);
        this->m_replyFd.store(replyPipe[0]);
        this->m_abortRequested.store(false);
        this->m_capabilitiesValid = false;
        detailCode = 0U;
        return true;
    }

    void reapPendingHelperLocked_() const {
        const pid_t pendingPid = this->m_reapPendingPid.load();
        if (pendingPid <= 0) {
            return;
        }
        int status = 0;
        const pid_t waitResult = ::waitpid(pendingPid, &status, WNOHANG);
        if (waitResult == pendingPid || (waitResult < 0 && errno == ECHILD)) {
            this->m_reapPendingPid.store(-1);
        }
    }

    void killHelperLocked_(int signalNumber) const {
        const int requestFd = this->m_requestFd.exchange(-1);
        if (requestFd >= 0) {
            ::close(requestFd);
        }
        const int replyFd = this->m_replyFd.exchange(-1);
        if (replyFd >= 0) {
            ::close(replyFd);
        }
        const pid_t pid = this->m_pid.load();
        if (pid > 0) {
            ::kill(pid, signalNumber);
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(250);
            while (std::chrono::steady_clock::now() < deadline) {
                int status = 0;
                const pid_t waitResult = ::waitpid(pid, &status, WNOHANG);
                if (waitResult == pid || (waitResult < 0 && errno == ECHILD)) {
                    this->m_pid.store(-1);
                    this->m_reapPendingPid.store(-1);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            if (this->m_pid.load() > 0) {
                this->m_pid.store(-1);
                this->m_reapPendingPid.store(pid);
            }
        }
        this->m_capabilitiesValid = false;
    }

    bool writeAllLocked_(const U8* bytes,
                         std::size_t length,
                         const std::chrono::steady_clock::time_point deadline,
                         const std::atomic<bool>* cancelRequested,
                         U32& detailCode) const {
        std::size_t offset = 0U;
        while (offset < length) {
            if (this->m_abortRequested.load() || (cancelRequested != nullptr && cancelRequested->load())) {
                detailCode = DETAIL_HELPER_ABORTED;
                return false;
            }
            const int requestFd = this->m_requestFd.load();
            if (requestFd < 0) {
                detailCode = this->m_abortRequested.load() ? DETAIL_HELPER_ABORTED : DETAIL_HELPER_WRITE_FAILED;
                return false;
            }
            if (!waitForFd_(requestFd, POLLOUT, deadline)) {
                detailCode = (this->m_abortRequested.load() || (cancelRequested != nullptr && cancelRequested->load()))
                                 ? DETAIL_HELPER_ABORTED
                                 : DETAIL_HELPER_TIMEOUT;
                return false;
            }
            const ssize_t written = ::send(
                requestFd,
                bytes + offset,
                length - offset,
#ifdef MSG_NOSIGNAL
                MSG_NOSIGNAL
#else
                0
#endif
            );
            if (written < 0 && errno == EINTR) {
                continue;
            }
            if (written <= 0) {
                detailCode = this->m_abortRequested.load() ? DETAIL_HELPER_ABORTED : DETAIL_HELPER_WRITE_FAILED;
                return false;
            }
            offset += static_cast<std::size_t>(written);
        }
        return true;
    }

    bool readAllLocked_(U8* bytes,
                        std::size_t length,
                        const std::chrono::steady_clock::time_point deadline,
                        const std::atomic<bool>* cancelRequested,
                        U32& detailCode) const {
        std::size_t offset = 0U;
        while (offset < length) {
            if (this->m_abortRequested.load() || (cancelRequested != nullptr && cancelRequested->load())) {
                detailCode = DETAIL_HELPER_ABORTED;
                return false;
            }
            const int replyFd = this->m_replyFd.load();
            if (replyFd < 0) {
                detailCode = this->m_abortRequested.load() ? DETAIL_HELPER_ABORTED : DETAIL_HELPER_READ_FAILED;
                return false;
            }
            if (!waitForFd_(replyFd, POLLIN, deadline)) {
                detailCode = (this->m_abortRequested.load() || (cancelRequested != nullptr && cancelRequested->load()))
                                 ? DETAIL_HELPER_ABORTED
                                 : DETAIL_HELPER_TIMEOUT;
                return false;
            }
            const ssize_t count = ::read(replyFd, bytes + offset, length - offset);
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                detailCode = this->m_abortRequested.load() ? DETAIL_HELPER_ABORTED : DETAIL_HELPER_READ_FAILED;
                return false;
            }
            offset += static_cast<std::size_t>(count);
        }
        return true;
    }

  private:
    mutable std::mutex m_transactionMutex;
    mutable std::mutex m_stateMutex;
    mutable std::atomic<pid_t> m_pid;
    mutable std::atomic<pid_t> m_reapPendingPid;
    mutable std::atomic<int> m_requestFd;
    mutable std::atomic<int> m_replyFd;
    mutable std::atomic<bool> m_abortRequested;
    mutable OBC::PayloadCapabilities m_cachedCapabilities;
    mutable bool m_capabilitiesValid;
};

}  // namespace

std::unique_ptr<OBC::IPiCameraDriver> makeDefaultPiCameraDriver() {
    return std::unique_ptr<OBC::IPiCameraDriver>(new PiCameraBackendHelperClient());
}

}  // namespace OBC
