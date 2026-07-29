#ifndef OBC_FILEINGRESSAUTHORITY_TESTER_HPP
#define OBC_FILEINGRESSAUTHORITY_TESTER_HPP

#include <array>
#include <vector>

#include "Fw/FilePacket/FilePacket.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"
#include "OBC/Components/FileIngressAuthority/FileIngressAuthority.hpp"
#include "OBC/Components/FileIngressAuthority/FileIngressAuthorityGTestBase.hpp"
#include "OBC/Components/test/TestSupport.hpp"

namespace OBC {

class FileIngressAuthorityTester final : public FileIngressAuthorityGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 32;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    struct BufferCapture {
        FwIndexType portNum;
        std::vector<U8> bytes;
    };

    struct AuthActivityCapture {
        SecureAuthActivity activity;
    };

    FileIngressAuthorityTester();
    ~FileIngressAuthorityTester() override;

    void testStartRequiresAuthAndAllowedPolicy();
    void testUhfBackupDeniedWithActiveAuth();
    void testRevocationDropsPacketsUntilNewStart();
    void testNonStagingDestinationRejected();
    void testSecondIngressReturnRoutesToOriginalSource();

  private:
    void connectPorts();
    void initComponents();

    void from_bufferSendOut_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
    void from_bufferReturnOut_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
    void from_authActivityOut_handler(FwIndexType portNum, const SecureAuthActivity& activity) override;

    Fw::Buffer makeWireBuffer(const Fw::FilePacket& packet);
    Fw::FilePacket makeStartPacket(const char* destinationPath) const;
    Fw::FilePacket makeDataPacket(U32 sequenceIndex) const;
    std::string decodeStartDestination(const BufferCapture& capture) const;
    void grantAuth(FwIndexType ingressPort, U32 serviceId);
    void revokeAuth(FwIndexType ingressPort, U32 serviceId, U32 reason);
    void setPolicy(FwIndexType ingressPort, AuthorityLinkIdentity identity, AuthorityLinkRole role, bool fileAllowed);
    void clearCaptures();

  private:
    TestSupport::TempDirectory m_runtimeRoot;
    std::array<U8, 4> m_dataBytes = {1U, 2U, 3U, 4U};
    std::vector<U8> m_scratch;
    FileIngressAuthority component;
    std::vector<BufferCapture> m_forwarded;
    std::vector<BufferCapture> m_returned;
    std::vector<AuthActivityCapture> m_authActivity;
};

}  // namespace OBC

#endif
