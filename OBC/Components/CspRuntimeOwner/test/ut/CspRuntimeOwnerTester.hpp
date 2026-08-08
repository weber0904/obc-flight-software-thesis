#ifndef OBC_COMPONENTS_CSPRUNTIMEOWNER_TEST_UT_CSPRUNTIMEOWNERTESTER_HPP
#define OBC_COMPONENTS_CSPRUNTIMEOWNER_TEST_UT_CSPRUNTIMEOWNERTESTER_HPP

#include <cstdint>
#include <mutex>
#include <vector>

#include "OBC/Components/CspRuntimeOwner/CspRuntimeOwner.hpp"
#include "OBC/Components/CspRuntimeOwner/CspRuntimeOwnerGTestBase.hpp"

namespace OBC {

class FakeOwnerRuntime final : public OBC::CSP::ICspRuntime {
  public:
    OBC::CSP::RuntimeStatus initStatus = OBC::CSP::RuntimeStatus::OK;
    OBC::CSP::RuntimeStatus pingStatus = OBC::CSP::RuntimeStatus::OK;
    OBC::CSP::RuntimeStatus sendStatus = OBC::CSP::RuntimeStatus::OK;
    OBC::CSP::RuntimeStatus requestReplyStatus = OBC::CSP::RuntimeStatus::OK;
    bool pingSuccess = true;
    OBC::CSP::RuntimeMetrics runtimeMetrics = {};
    std::vector<std::uint8_t> replyBytes = {};

    OBC::CSP::RuntimeStatus init(const OBC::CSP::RuntimeConfig& config) override;
    OBC::CSP::RuntimeStatus ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) override;
    OBC::CSP::RuntimeStatus sendRaw(std::uint16_t targetNode,
                                    std::uint8_t targetPort,
                                    const std::string& data) override;
    OBC::CSP::RuntimeStatus requestReply(std::uint16_t targetNode,
                                         std::uint8_t targetPort,
                                         const void* requestData,
                                         std::size_t requestSize,
                                         void* replyData,
                                         std::size_t replyCapacity,
                                         std::size_t& replySize,
                                         std::uint32_t timeoutMs) override;
    OBC::CSP::RuntimeMetrics metrics() const override;
    void shutdown() override;

    U32 getPingCalls() const;
    U32 getRequestReplyCalls() const;
    U32 getShutdownCalls() const;

  private:
    mutable std::mutex m_mutex;
    U32 m_pingCalls = 0U;
    U32 m_requestReplyCalls = 0U;
    U32 m_shutdownCalls = 0U;
};

class CspRuntimeOwnerTester final : public CspRuntimeOwnerGTestBase {
  public:
    static constexpr FwSizeType MAX_HISTORY_SIZE = 32;
    static constexpr FwEnumStoreType TEST_INSTANCE_ID = 0;

    CspRuntimeOwnerTester();
    ~CspRuntimeOwnerTester() override;

    void testSyncRequestReplyCopiesReplyBuffer();
    void testAsyncPingCompletionReturnsWorkerResult();
    void testAsyncRequestReplyCompletionReturnsReply();
    void testTimeoutPublishesEventAndTelemetry();
    void testStopWorkerRejectsNewRequests();

  private:
    void connectPorts();
    void initComponents();

    bool waitForAsyncPingCompletion_(std::uint64_t handle, OBC::AsyncCspPingCompletion& completion);
    bool waitForAsyncRequestReplyCompletion_(std::uint64_t handle, OBC::AsyncCspRequestReplyCompletion& completion);

  private:
    FakeOwnerRuntime m_runtime;
    CspRuntimeOwner component;
};

}  // namespace OBC

#endif
