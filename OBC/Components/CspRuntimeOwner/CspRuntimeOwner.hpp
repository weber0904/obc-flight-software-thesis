#ifndef OBC_COMPONENTS_CSPRUNTIMEOWNER_HPP
#define OBC_COMPONENTS_CSPRUNTIMEOWNER_HPP

#include "OBC/Components/CspRuntimeOwner/AsyncCspRuntimeOwner.hpp"
#include "OBC/Components/CspRuntimeOwner/CspRuntimeOwnerComponentAc.hpp"
#include "simulators/csp/CspRuntime.hpp"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace OBC {

class CspRuntimeOwner final : public CspRuntimeOwnerComponentBase,
                              public OBC::CSP::ICspRuntime,
                              public IAsyncCspRuntimeOwner {
  public:
    using CspRuntimeOwnerComponentBase::init;

    explicit CspRuntimeOwner(const char* const compName);
    CspRuntimeOwner(const char* const compName, OBC::CSP::ICspRuntime& runtime);
    ~CspRuntimeOwner() override;

    OBC::CSP::RuntimeStatus init(const OBC::CSP::RuntimeConfig& config) override;
    OBC::CSP::RuntimeStatus ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) override;
    OBC::CSP::RuntimeStatus sendRaw(std::uint16_t targetNode, std::uint8_t targetPort, const std::string& data) override;
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

    bool submitAsyncPing(std::uint16_t targetNode, std::uint32_t timeoutMs, std::uint64_t& handle) override;
    bool takeAsyncPingCompletion(std::uint64_t handle, AsyncCspPingCompletion& completion) override;

    bool submitAsyncRequestReply(std::uint16_t targetNode,
                                 std::uint8_t targetPort,
                                 const void* requestData,
                                 std::size_t requestSize,
                                 std::size_t replyCapacity,
                                 std::uint32_t timeoutMs,
                                 std::uint64_t& handle) override;
    bool takeAsyncRequestReplyCompletion(std::uint64_t handle, AsyncCspRequestReplyCompletion& completion) override;

    void recordCoalescedForRuntime() override;
    void stopWorkerForRuntime();

  private:
    enum class RequestKind {
        INIT,
        PING,
        SEND_RAW,
        REQUEST_REPLY,
        SHUTDOWN,
    };

    struct Request {
        RequestKind kind = RequestKind::PING;
        bool async = false;
        std::uint64_t handle = 0U;
        std::uint16_t targetNode = 0U;
        std::uint8_t targetPort = 0U;
        std::uint32_t timeoutMs = 0U;
        OBC::CSP::RuntimeConfig config = {};
        std::vector<std::uint8_t> request;
        std::size_t replyCapacity = 0U;
        std::condition_variable* completionCv = nullptr;
        bool* completionFlag = nullptr;
        OBC::CSP::RuntimeStatus* syncStatus = nullptr;
        bool* syncPingSuccess = nullptr;
        std::vector<std::uint8_t>* syncReply = nullptr;
        std::size_t* syncReplySize = nullptr;
    };

    void workerLoop_();
    bool enqueueSyncRequest_(Request& request);
    bool enqueueAsyncRequest_(Request& request, std::uint64_t& handle);
    void processRequest_(Request& request);
    void refreshTelemetry_();
    void noteTimeout_(std::uint16_t targetNode, std::uint8_t targetPort, std::uint32_t timeoutMs);

  private:
    mutable std::mutex m_mutex;
    std::condition_variable m_queueCv;
    std::thread m_worker;
    bool m_workerRunning;
    bool m_shutdownRequested;
    std::uint64_t m_nextHandle;
    std::deque<Request> m_queue;
    std::unordered_map<std::uint64_t, AsyncCspPingCompletion> m_asyncPingCompletions;
    std::unordered_map<std::uint64_t, AsyncCspRequestReplyCompletion> m_asyncRequestReplyCompletions;
    U32 m_queueDepthTlm;
    U32 m_inflightTlm;
    U32 m_totalTimeouts;
    U32 m_totalCoalesced;
    U32 m_lastLatencyUsec;
    U32 m_lastResult;
    bool m_liveSuccessTelemetryEnabled;
    OBC::CSP::LibCspRuntime m_defaultRuntime;
    OBC::CSP::ICspRuntime* m_runtime;
};

}  // namespace OBC

#endif
