#ifndef OBC_CSP_RUNTIME_HPP
#define OBC_CSP_RUNTIME_HPP

#include <cstdint>
#include <cstddef>
#include <string>

namespace OBC {
namespace CSP {

struct RuntimeConfig {
    std::uint16_t nodeId = 1U;
    std::string transportKind = "zmqhub";
    std::string hubHost = "127.0.0.1";
    std::uint16_t hubSubPort = 6100U;
    std::uint16_t hubPubPort = 7100U;
    std::string canDevice = "";
    bool canPromisc = false;
    std::string interfaceName = "OBCCSP";
};

struct RuntimeMetrics {
    bool initialized = false;
    std::uint16_t localNodeId = 0U;
    std::uint32_t txPackets = 0U;
    std::uint32_t rxPackets = 0U;
    std::uint32_t errorCount = 0U;
    std::uint32_t freeBuffers = 0U;
};

enum class RuntimeStatus {
    OK,
    INVALID_ARGUMENT,
    TIMEOUT,
    EXECUTION_ERROR,
};

class ICspRuntime {
  public:
    virtual ~ICspRuntime() = default;

    virtual RuntimeStatus init(const RuntimeConfig& config) = 0;
    virtual RuntimeStatus ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) = 0;
    virtual RuntimeStatus sendRaw(std::uint16_t targetNode, std::uint8_t targetPort, const std::string& data) = 0;
    virtual RuntimeStatus requestReply(std::uint16_t targetNode,
                                       std::uint8_t targetPort,
                                       const void* requestData,
                                       std::size_t requestSize,
                                       void* replyData,
                                       std::size_t replyCapacity,
                                       std::size_t& replySize,
                                       std::uint32_t timeoutMs) = 0;
    virtual RuntimeMetrics metrics() const = 0;
    virtual void shutdown() = 0;
};

class LibCspRuntime final : public ICspRuntime {
  public:
    LibCspRuntime();
    ~LibCspRuntime() override;

    RuntimeStatus init(const RuntimeConfig& config) override;
    RuntimeStatus ping(std::uint16_t targetNode, std::uint32_t timeoutMs, bool& success) override;
    RuntimeStatus sendRaw(std::uint16_t targetNode, std::uint8_t targetPort, const std::string& data) override;
    RuntimeStatus requestReply(std::uint16_t targetNode,
                               std::uint8_t targetPort,
                               const void* requestData,
                               std::size_t requestSize,
                               void* replyData,
                               std::size_t replyCapacity,
                               std::size_t& replySize,
                               std::uint32_t timeoutMs) override;
    RuntimeMetrics metrics() const override;
    void shutdown() override;

  private:
    struct Impl;
    static RuntimeMetrics snapshotMetrics_(const Impl& impl);
    static void storeCachedMetrics_(Impl& impl, const RuntimeMetrics& metrics);
    static RuntimeMetrics loadCachedMetrics_(const Impl& impl);
    static void refreshCachedMetrics_(Impl& impl);
    Impl* m_impl;
};

RuntimeConfig runtimeConfigFromEnvironment(std::uint16_t nodeId, const char* interfaceName);
ICspRuntime& defaultRuntime();

}  // namespace CSP
}  // namespace OBC

#endif
