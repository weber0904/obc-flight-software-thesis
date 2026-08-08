#ifndef OBC_COMPONENTS_ASYNCCSPRUNTIMEOWNER_HPP
#define OBC_COMPONENTS_ASYNCCSPRUNTIMEOWNER_HPP

#include "simulators/csp/CspRuntime.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace OBC {

struct AsyncCspPingCompletion {
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    bool success = false;
    std::uint32_t latencyUsec = 0U;
};

struct AsyncCspRequestReplyCompletion {
    OBC::CSP::RuntimeStatus status = OBC::CSP::RuntimeStatus::EXECUTION_ERROR;
    std::vector<std::uint8_t> reply;
    std::size_t replySize = 0U;
    std::uint32_t latencyUsec = 0U;
};

class IAsyncCspRuntimeOwner {
  public:
    virtual ~IAsyncCspRuntimeOwner() = default;

    virtual bool submitAsyncPing(std::uint16_t targetNode, std::uint32_t timeoutMs, std::uint64_t& handle) = 0;
    virtual bool takeAsyncPingCompletion(std::uint64_t handle, AsyncCspPingCompletion& completion) = 0;

    virtual bool submitAsyncRequestReply(std::uint16_t targetNode,
                                         std::uint8_t targetPort,
                                         const void* requestData,
                                         std::size_t requestSize,
                                         std::size_t replyCapacity,
                                         std::uint32_t timeoutMs,
                                         std::uint64_t& handle) = 0;
    virtual bool takeAsyncRequestReplyCompletion(std::uint64_t handle, AsyncCspRequestReplyCompletion& completion) = 0;

    virtual void recordCoalescedForRuntime() = 0;
};

}  // namespace OBC

#endif
