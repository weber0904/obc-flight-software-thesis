#include "CspRuntimeOwnerTester.hpp"

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

namespace OBC {

OBC::CSP::RuntimeStatus FakeOwnerRuntime::init(const OBC::CSP::RuntimeConfig& config) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    if (this->initStatus == OBC::CSP::RuntimeStatus::OK) {
        this->runtimeMetrics.initialized = true;
        this->runtimeMetrics.localNodeId = config.nodeId;
    }
    return this->initStatus;
}

OBC::CSP::RuntimeStatus FakeOwnerRuntime::ping(std::uint16_t, std::uint32_t, bool& success) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_pingCalls += 1U;
    if (this->pingStatus != OBC::CSP::RuntimeStatus::OK) {
        success = false;
        return this->pingStatus;
    }
    success = this->pingSuccess;
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeStatus FakeOwnerRuntime::sendRaw(std::uint16_t, std::uint8_t, const std::string&) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->sendStatus;
}

OBC::CSP::RuntimeStatus FakeOwnerRuntime::requestReply(std::uint16_t,
                                                       std::uint8_t,
                                                       const void*,
                                                       std::size_t,
                                                       void* replyData,
                                                       std::size_t replyCapacity,
                                                       std::size_t& replySize,
                                                       std::uint32_t) {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_requestReplyCalls += 1U;
    if (this->requestReplyStatus != OBC::CSP::RuntimeStatus::OK) {
        replySize = 0U;
        return this->requestReplyStatus;
    }

    replySize = std::min(this->replyBytes.size(), replyCapacity);
    if (replyData != nullptr && replySize > 0U) {
        std::memcpy(replyData, this->replyBytes.data(), replySize);
    }
    return OBC::CSP::RuntimeStatus::OK;
}

OBC::CSP::RuntimeMetrics FakeOwnerRuntime::metrics() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->runtimeMetrics;
}

void FakeOwnerRuntime::shutdown() {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    this->m_shutdownCalls += 1U;
    this->runtimeMetrics.initialized = false;
}

U32 FakeOwnerRuntime::getPingCalls() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_pingCalls;
}

U32 FakeOwnerRuntime::getRequestReplyCalls() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_requestReplyCalls;
}

U32 FakeOwnerRuntime::getShutdownCalls() const {
    std::lock_guard<std::mutex> lock(this->m_mutex);
    return this->m_shutdownCalls;
}

CspRuntimeOwnerTester::CspRuntimeOwnerTester()
    : CspRuntimeOwnerGTestBase("CspRuntimeOwnerTester", MAX_HISTORY_SIZE),
      m_runtime(),
      component("CspRuntimeOwner", this->m_runtime) {
    this->initComponents();
    this->connectPorts();
}

CspRuntimeOwnerTester::~CspRuntimeOwnerTester() = default;

void CspRuntimeOwnerTester::testSyncRequestReplyCopiesReplyBuffer() {
    this->m_runtime.replyBytes = {0x10U, 0x20U, 0x30U};
    std::uint8_t reply[8] = {};
    std::size_t replySize = 0U;

    const OBC::CSP::RuntimeStatus status =
        this->component.requestReply(5U, 9U, "abc", 3U, reply, sizeof(reply), replySize, 25U);

    ASSERT_EQ(status, OBC::CSP::RuntimeStatus::OK);
    ASSERT_EQ(replySize, 3U);
    ASSERT_EQ(this->m_runtime.getRequestReplyCalls(), 1U);
    ASSERT_EQ(reply[0], 0x10U);
    ASSERT_EQ(reply[1], 0x20U);
    ASSERT_EQ(reply[2], 0x30U);
}

void CspRuntimeOwnerTester::testAsyncPingCompletionReturnsWorkerResult() {
    std::uint64_t handle = 0U;
    ASSERT_TRUE(this->component.submitAsyncPing(7U, 50U, handle));

    OBC::AsyncCspPingCompletion completion = {};
    ASSERT_TRUE(this->waitForAsyncPingCompletion_(handle, completion));
    ASSERT_EQ(this->m_runtime.getPingCalls(), 1U);
    ASSERT_EQ(completion.status, OBC::CSP::RuntimeStatus::OK);
    ASSERT_TRUE(completion.success);

    OBC::AsyncCspPingCompletion duplicate = {};
    ASSERT_FALSE(this->component.takeAsyncPingCompletion(handle, duplicate));
}

void CspRuntimeOwnerTester::testAsyncRequestReplyCompletionReturnsReply() {
    this->m_runtime.replyBytes = {0xAAU, 0xBBU};
    std::uint64_t handle = 0U;
    const std::uint8_t request[] = {0x01U, 0x02U};
    ASSERT_TRUE(this->component.submitAsyncRequestReply(4U, 3U, request, sizeof(request), 8U, 40U, handle));

    OBC::AsyncCspRequestReplyCompletion completion = {};
    ASSERT_TRUE(this->waitForAsyncRequestReplyCompletion_(handle, completion));
    ASSERT_EQ(this->m_runtime.getRequestReplyCalls(), 1U);
    ASSERT_EQ(completion.status, OBC::CSP::RuntimeStatus::OK);
    ASSERT_EQ(completion.replySize, 2U);
    ASSERT_EQ(completion.reply.size(), 2U);
    ASSERT_EQ(completion.reply[0], 0xAAU);
    ASSERT_EQ(completion.reply[1], 0xBBU);
}

void CspRuntimeOwnerTester::testTimeoutPublishesEventAndTelemetry() {
    this->m_runtime.pingStatus = OBC::CSP::RuntimeStatus::TIMEOUT;
    bool success = true;

    this->clearHistory();
    const OBC::CSP::RuntimeStatus status = this->component.ping(9U, 123U, success);

    ASSERT_EQ(status, OBC::CSP::RuntimeStatus::TIMEOUT);
    ASSERT_FALSE(success);
    ASSERT_EVENTS_CSP_OWNER_TIMEOUT_SIZE(1);
    ASSERT_EVENTS_CSP_OWNER_TIMEOUT(0, 9U, 0U, 123U);
    ASSERT_TLM_CSP_OWNER_TOTAL_TIMEOUTS_SIZE(1);
    ASSERT_TLM_CSP_OWNER_TOTAL_TIMEOUTS(0, 1U);
    ASSERT_TLM_CSP_OWNER_LAST_RESULT_SIZE(1);
    ASSERT_TLM_CSP_OWNER_LAST_RESULT(0, static_cast<U32>(OBC::CSP::RuntimeStatus::TIMEOUT));
}

void CspRuntimeOwnerTester::testStopWorkerRejectsNewRequests() {
    this->component.stopWorkerForRuntime();

    bool success = true;
    const OBC::CSP::RuntimeStatus status = this->component.ping(11U, 10U, success);
    ASSERT_EQ(status, OBC::CSP::RuntimeStatus::EXECUTION_ERROR);
    ASSERT_TRUE(success);
    ASSERT_EQ(this->m_runtime.getPingCalls(), 0U);

    std::uint64_t handle = 0U;
    ASSERT_FALSE(this->component.submitAsyncPing(11U, 10U, handle));
}

bool CspRuntimeOwnerTester::waitForAsyncPingCompletion_(std::uint64_t handle, OBC::AsyncCspPingCompletion& completion) {
    for (U32 attempt = 0U; attempt < 200U; attempt++) {
        if (this->component.takeAsyncPingCompletion(handle, completion)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

bool CspRuntimeOwnerTester::waitForAsyncRequestReplyCompletion_(
    std::uint64_t handle,
    OBC::AsyncCspRequestReplyCompletion& completion) {
    for (U32 attempt = 0U; attempt < 200U; attempt++) {
        if (this->component.takeAsyncRequestReplyCompletion(handle, completion)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

}  // namespace OBC
