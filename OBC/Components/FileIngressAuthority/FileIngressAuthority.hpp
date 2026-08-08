#ifndef OBC_FILE_INGRESS_AUTHORITY_HPP
#define OBC_FILE_INGRESS_AUTHORITY_HPP

#include "OBC/Components/FileIngressAuthority/FileIngressAuthorityComponentAc.hpp"
#include "OBC/Components/FileIngressAuthority/FileIngressPolicy.hpp"
#include "OBC/Components/FileIngressControlProtocol/FileIngressPolicyStateSerializableAc.hpp"

#include <array>
#include <unordered_map>

namespace OBC {

class FileIngressAuthority final : public FileIngressAuthorityComponentBase {
  public:
    explicit FileIngressAuthority(const char* compName);
    ~FileIngressAuthority() override;

    bool configureRuntime(const std::string& runtimeRoot, const std::string& workingRoot);

    const FileIngressPolicy& getPolicy() const;

  private:
    struct IngressRuntimeState {
        bool dropTransfer = false;
        bool secureAuthActive = false;
        U8 serviceId = 0U;
        bool fileAllowed = false;
    };

    void bufferSendIn_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
    void bufferReturnIn_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
    void authGrantedIn_handler(FwIndexType portNum, const SecureAuthGrant& grant) override;
    void authRevokedIn_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) override;
    void filePolicyIn_handler(FwIndexType portNum, const FileIngressPolicyState& policy) override;

    void returnBuffer(Fw::Buffer& buffer, FwIndexType portNum);
    void rememberForwardSource_(const Fw::Buffer& buffer, FwIndexType portNum);
    FwIndexType forgetForwardSource_(const Fw::Buffer& buffer);
    void forwardBuffer(Fw::Buffer& buffer, FwIndexType portNum);
    void setDropTransfer_(FwIndexType portNum, bool enabled);
    bool isAnyDropTransferActive_() const;
    void refreshAuthActivity_(FwIndexType portNum);
    U32 allowedPortCount_() const;

  private:
    FileIngressPolicy m_policy;
    std::unordered_map<const U8*, FwIndexType> m_forwardOrigins;
    std::array<IngressRuntimeState, 2> m_ingressState = {};
    U32 m_acceptedStarts = 0U;
    U32 m_rejectedStarts = 0U;
    U32 m_droppedPackets = 0U;
};

}  // namespace OBC

#endif
