#include "OBC/Components/CspRuntimeOwner/CspRuntimeOwner.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>

namespace OBC {

namespace {

U32 toResultCode(OBC::CSP::RuntimeStatus status) {
    return static_cast<U32>(status);
}

bool readBoolEnv(const char* const name, bool fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }
    return std::strcmp(value, "0") != 0 && std::strcmp(value, "false") != 0 &&
           std::strcmp(value, "False") != 0 && std::strcmp(value, "no") != 0 &&
           std::strcmp(value, "NO") != 0;
}

}  // namespace

CspRuntimeOwner::CspRuntimeOwner(const char* const compName)
    : CspRuntimeOwnerComponentBase(compName),
      m_worker(),
      m_workerRunning(true),
      m_shutdownRequested(false),
      m_nextHandle(1U),
      m_queue(),
      m_asyncPingCompletions(),
      m_asyncRequestReplyCompletions(),
      m_queueDepthTlm(0U),
      m_inflightTlm(0U),
      m_totalTimeouts(0U),
      m_totalCoalesced(0U),
      m_lastLatencyUsec(0U),
      m_lastResult(toResultCode(OBC::CSP::RuntimeStatus::OK)),
      m_liveSuccessTelemetryEnabled(readBoolEnv("CSP_OWNER_LIVE_SUCCESS_TELEMETRY", false)),
      m_defaultRuntime(),
      m_runtime(&m_defaultRuntime) {
    this->m_worker = std::thread([this]() { this->workerLoop_(); });
}

CspRuntimeOwner::CspRuntimeOwner(const char* const compName, OBC::CSP::ICspRuntime& runtime)
    : CspRuntimeOwnerComponentBase(compName),
      m_worker(),
      m_workerRunning(true),
      m_shutdownRequested(false),
      m_nextHandle(1U),
      m_queue(),
      m_asyncPingCompletions(),
      m_asyncRequestReplyCompletions(),
      m_queueDepthTlm(0U),
      m_inflightTlm(0U),
      m_totalTimeouts(0U),
      m_totalCoalesced(0U),
      m_lastLatencyUsec(0U),
      m_lastResult(toResultCode(OBC::CSP::RuntimeStatus::OK)),
      m_liveSuccessTelemetryEnabled(readBoolEnv("CSP_OWNER_LIVE_SUCCESS_TELEMETRY", false)),
      m_defaultRuntime(),
      m_runtime(&runtime) {
    this->m_worker = std::thread([this]() { this->workerLoop_(); });
}

CspRuntimeOwner::~CspRuntimeOwner() {
    this->stopWorkerForRuntime();
}

OBC::CSP::RuntimeStatus CspRuntimeOwner::init(const OBC::CSP::RuntimeConfig& config) {
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    bool completion = false;
    std::condition_variable cv;
    Request request = {};
    request.kind = RequestKind::INIT;
    request.config = config;
    request.completionCv = &cv;
    request.completionFlag = &completion;
    request.syncStatus = &status;
    if (!this->enqueueSyncRequest_(request)) {
        return status;
    }
    std::unique_lock<std::mutex> lock(this->m_mutex);
    cv.wait(lock, [&completion]() { return completion; });
    return status;
}

OBC::CSP::RuntimeStatus CspRuntimeOwner::ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) {
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    bool completion = false;
    std::condition_variable cv;
    Request request = {};
    request.kind = RequestKind::PING;
    request.targetNode = targetNode;
    request.timeoutMs = timeoutMs;
    request.completionCv = &cv;
    request.completionFlag = &completion;
    request.syncStatus = &status;
    request.syncPingSuccess = &success;
    if (!this->enqueueSyncRequest_(request)) {
        return status;
    }
    std::unique_lock<std::mutex> lock(this->m_mutex);
    cv.wait(lock, [&completion]() { return completion; });
    return status;
}

OBC::CSP::RuntimeStatus CspRuntimeOwner::sendRaw(std::uint16_t targetNode,
                                                 std::uint8_t targetPort,
                                                 const std::string& data) {
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    bool completion = false;
    std::condition_variable cv;
    Request request = {};
    request.kind = RequestKind::SEND_RAW;
    request.targetNode = targetNode;
    request.targetPort = targetPort;
    request.request.assign(data.begin(), data.end());
    request.completionCv = &cv;
    request.completionFlag = &completion;
    request.syncStatus = &status;
    if (!this->enqueueSyncRequest_(request)) {
        return status;
    }
    std::unique_lock<std::mutex> lock(this->m_mutex);
    cv.wait(lock, [&completion]() { return completion; });
    return status;
}

OBC::CSP::RuntimeStatus CspRuntimeOwner::requestReply(std::uint16_t targetNode,
                                                      std::uint8_t targetPort,
                                                      const void* requestData,
                                                      std::size_t requestSize,
                                                      void* replyData,
                                                      std::size_t replyCapacity,
                                                      std::size_t& replySize,
                                                      std::uint32_t timeoutMs) {
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    bool completion = false;
    std::condition_variable cv;
    std::vector<std::uint8_t> reply;
    Request request = {};
    request.kind = RequestKind::REQUEST_REPLY;
    request.targetNode = targetNode;
    request.targetPort = targetPort;
    request.timeoutMs = timeoutMs;
    request.replyCapacity = replyCapacity;
    request.completionCv = &cv;
    request.completionFlag = &completion;
    request.syncStatus = &status;
    request.syncReply = &reply;
    request.syncReplySize = &replySize;
    if (requestData != nullptr && requestSize > 0U) {
        const auto* bytes = static_cast<const std::uint8_t*>(requestData);
        request.request.assign(bytes, bytes + requestSize);
    }
    if (!this->enqueueSyncRequest_(request)) {
        replySize = 0U;
        return status;
    }
    std::unique_lock<std::mutex> lock(this->m_mutex);
    cv.wait(lock, [&completion]() { return completion; });
    if (status == OBC::CSP::RuntimeStatus::OK && replyData != nullptr && !reply.empty()) {
        std::memcpy(replyData, reply.data(), std::min(reply.size(), replyCapacity));
    }
    return status;
}

OBC::CSP::RuntimeMetrics CspRuntimeOwner::metrics() const {
    return this->m_runtime->metrics();
}

void CspRuntimeOwner::shutdown() {
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    bool completion = false;
    std::condition_variable cv;
    Request request = {};
    request.kind = RequestKind::SHUTDOWN;
    request.completionCv = &cv;
    request.completionFlag = &completion;
    request.syncStatus = &status;
    if (!this->enqueueSyncRequest_(request)) {
        return;
    }
    std::unique_lock<std::mutex> lock(this->m_mutex);
    cv.wait(lock, [&completion]() { return completion; });
}

bool CspRuntimeOwner::submitAsyncPing(std::uint16_t targetNode, std::uint32_t timeoutMs, std::uint64_t& handle) {
    Request request = {};
    request.kind = RequestKind::PING;
    request.async = true;
    request.targetNode = targetNode;
    request.timeoutMs = timeoutMs;
    return this->enqueueAsyncRequest_(request, handle);
}

bool CspRuntimeOwner::takeAsyncPingCompletion(std::uint64_t handle, AsyncCspPingCompletion& completion) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    const auto it = this->m_asyncPingCompletions.find(handle);
    if (it == this->m_asyncPingCompletions.end()) {
        return false;
    }
    completion = it->second;
    this->m_asyncPingCompletions.erase(it);
    return true;
}

bool CspRuntimeOwner::submitAsyncRequestReply(std::uint16_t targetNode,
                                              std::uint8_t targetPort,
                                              const void* requestData,
                                              std::size_t requestSize,
                                              std::size_t replyCapacity,
                                              std::uint32_t timeoutMs,
                                              std::uint64_t& handle) {
    Request request = {};
    request.kind = RequestKind::REQUEST_REPLY;
    request.async = true;
    request.targetNode = targetNode;
    request.targetPort = targetPort;
    request.timeoutMs = timeoutMs;
    request.replyCapacity = replyCapacity;
    if (requestData != nullptr && requestSize > 0U) {
        const auto* bytes = static_cast<const std::uint8_t*>(requestData);
        request.request.assign(bytes, bytes + requestSize);
    }
    return this->enqueueAsyncRequest_(request, handle);
}

bool CspRuntimeOwner::takeAsyncRequestReplyCompletion(std::uint64_t handle, AsyncCspRequestReplyCompletion& completion) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    const auto it = this->m_asyncRequestReplyCompletions.find(handle);
    if (it == this->m_asyncRequestReplyCompletions.end()) {
        return false;
    }
    completion = it->second;
    this->m_asyncRequestReplyCompletions.erase(it);
    return true;
}

void CspRuntimeOwner::recordCoalescedForRuntime() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_totalCoalesced += 1U;
    this->tlmWrite_CSP_OWNER_TOTAL_COALESCED(this->m_totalCoalesced);
}

void CspRuntimeOwner::stopWorkerForRuntime() {
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        if (!this->m_workerRunning) {
            return;
        }
        this->m_workerRunning = false;
        this->m_shutdownRequested = true;
    }
    this->m_queueCv.notify_all();
    if (this->m_worker.joinable()) {
        this->m_worker.join();
    }
}

void CspRuntimeOwner::workerLoop_() {
    while (true) {
        Request request = {};
        {
            std::unique_lock<std::mutex> lock(this->m_mutex);
            this->m_queueCv.wait(lock, [this]() { return this->m_shutdownRequested || !this->m_queue.empty(); });
            if (this->m_shutdownRequested && this->m_queue.empty()) {
                return;
            }
            request = std::move(this->m_queue.front());
            this->m_queue.pop_front();
            this->m_queueDepthTlm = static_cast<U32>(this->m_queue.size());
            this->m_inflightTlm = 1U;
            this->refreshTelemetry_();
        }
        this->processRequest_(request);
        {
            std::lock_guard<std::mutex> lock(this->m_mutex);
            this->m_inflightTlm = 0U;
            this->refreshTelemetry_();
        }
    }
}

bool CspRuntimeOwner::enqueueSyncRequest_(Request& request) {
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        if (!this->m_workerRunning) {
            return false;
        }
        this->m_queue.push_back(request);
        this->m_queueDepthTlm = static_cast<U32>(this->m_queue.size());
        this->refreshTelemetry_();
    }
    this->m_queueCv.notify_one();
    return true;
}

bool CspRuntimeOwner::enqueueAsyncRequest_(Request& request, std::uint64_t& handle) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (!this->m_workerRunning) {
        return false;
    }
    request.handle = this->m_nextHandle++;
    handle = request.handle;
    this->m_queue.push_back(std::move(request));
    this->m_queueDepthTlm = static_cast<U32>(this->m_queue.size());
    this->refreshTelemetry_();
    this->m_queueCv.notify_one();
    return true;
}

void CspRuntimeOwner::processRequest_(Request& request) {
    const auto started = std::chrono::steady_clock::now();
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    bool pingSuccess = false;
    std::vector<std::uint8_t> replyBuffer;
    std::size_t replySize = 0U;

    switch (request.kind) {
        case RequestKind::INIT:
            status = this->m_runtime->init(request.config);
            break;
        case RequestKind::PING:
            status = this->m_runtime->ping(request.targetNode, request.timeoutMs, pingSuccess);
            break;
        case RequestKind::SEND_RAW: {
            const std::string payload(request.request.begin(), request.request.end());
            status = this->m_runtime->sendRaw(request.targetNode, request.targetPort, payload);
            break;
        }
        case RequestKind::REQUEST_REPLY:
            replyBuffer.resize(request.replyCapacity, 0U);
            status = this->m_runtime->requestReply(request.targetNode,
                                                   request.targetPort,
                                                   request.request.data(),
                                                   request.request.size(),
                                                   replyBuffer.data(),
                                                   replyBuffer.size(),
                                                   replySize,
                                                   request.timeoutMs);
            replyBuffer.resize(replySize);
            break;
        case RequestKind::SHUTDOWN:
            this->m_runtime->shutdown();
            status = OBC::CSP::RuntimeStatus::OK;
            break;
    }

    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                             std::chrono::steady_clock::now() - started)
                             .count();

    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_lastLatencyUsec = static_cast<U32>(std::min<long long>(elapsed, 0xFFFFFFFFLL));
    this->m_lastResult = toResultCode(status);
    if (status == OBC::CSP::RuntimeStatus::TIMEOUT) {
        this->noteTimeout_(request.targetNode, request.targetPort, request.timeoutMs);
    } else if (status != OBC::CSP::RuntimeStatus::OK || this->m_liveSuccessTelemetryEnabled) {
        this->tlmWrite_CSP_OWNER_LAST_LATENCY_USEC(this->m_lastLatencyUsec);
        this->tlmWrite_CSP_OWNER_LAST_RESULT(this->m_lastResult);
    }

    if (request.async) {
        if (request.kind == RequestKind::PING) {
            this->m_asyncPingCompletions[request.handle] =
                AsyncCspPingCompletion{status, pingSuccess, this->m_lastLatencyUsec};
        } else {
            this->m_asyncRequestReplyCompletions[request.handle] =
                AsyncCspRequestReplyCompletion{status, std::move(replyBuffer), replySize, this->m_lastLatencyUsec};
        }
        return;
    }

    if (request.syncStatus != nullptr) {
        *request.syncStatus = status;
    }
    if (request.syncPingSuccess != nullptr) {
        *request.syncPingSuccess = pingSuccess;
    }
    if (request.syncReply != nullptr) {
        *request.syncReply = std::move(replyBuffer);
    }
    if (request.syncReplySize != nullptr) {
        *request.syncReplySize = replySize;
    }
    if (request.completionFlag != nullptr) {
        *request.completionFlag = true;
    }
    if (request.completionCv != nullptr) {
        request.completionCv->notify_all();
    }
}

void CspRuntimeOwner::refreshTelemetry_() {
    if (!this->m_liveSuccessTelemetryEnabled) {
        return;
    }
    this->tlmWrite_CSP_OWNER_QUEUE_DEPTH(this->m_queueDepthTlm);
    this->tlmWrite_CSP_OWNER_INFLIGHT(this->m_inflightTlm);
    this->tlmWrite_CSP_OWNER_TOTAL_TIMEOUTS(this->m_totalTimeouts);
    this->tlmWrite_CSP_OWNER_TOTAL_COALESCED(this->m_totalCoalesced);
}

void CspRuntimeOwner::noteTimeout_(std::uint16_t targetNode, std::uint8_t targetPort, std::uint32_t timeoutMs) {
    this->m_totalTimeouts += 1U;
    this->log_WARNING_HI_CSP_OWNER_TIMEOUT(targetNode, targetPort, timeoutMs);
    this->tlmWrite_CSP_OWNER_TOTAL_TIMEOUTS(this->m_totalTimeouts);
    if (this->m_liveSuccessTelemetryEnabled) {
        this->refreshTelemetry_();
    }
    this->tlmWrite_CSP_OWNER_LAST_LATENCY_USEC(this->m_lastLatencyUsec);
    this->tlmWrite_CSP_OWNER_LAST_RESULT(this->m_lastResult);
}

}  // namespace OBC
