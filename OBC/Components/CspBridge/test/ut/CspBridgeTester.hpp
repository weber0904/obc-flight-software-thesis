#ifndef OBC_CspBridgeTester_HPP
#define OBC_CspBridgeTester_HPP

#include "OBC/Components/CspBridge/CspBridge.hpp"
#include "OBC/Components/CspBridge/CspBridgeGTestBase.hpp"
#include "simulators/csp/CspRuntime.hpp"

namespace OBC {

class FakeCspRuntime final : public OBC::CSP::ICspRuntime {
  public:
    OBC::CSP::RuntimeStatus initStatus = OBC::CSP::RuntimeStatus::OK;
    OBC::CSP::RuntimeStatus pingStatus = OBC::CSP::RuntimeStatus::OK;
    OBC::CSP::RuntimeStatus sendStatus = OBC::CSP::RuntimeStatus::OK;
    bool pingSuccess = true;
    OBC::CSP::RuntimeMetrics runtimeMetrics = {};

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
};

class CspBridgeTester final : public CspBridgeGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 10;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    CspBridgeTester();

    ~CspBridgeTester() override;

    void testInitAndSuccessfulPing();

    void testSendBeforeInitFails();

    void testPingFailureReturnsFalseResult();

  private:
    void connectPorts();

    void initComponents();

  private:
    OBC::FakeCspRuntime runtime;
    OBC::CspBridge component;
};

}  // namespace OBC

#endif
