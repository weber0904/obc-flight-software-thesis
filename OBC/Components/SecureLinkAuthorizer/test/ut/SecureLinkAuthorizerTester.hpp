#ifndef OBC_SECURELINKAUTHORIZER_TESTER_HPP
#define OBC_SECURELINKAUTHORIZER_TESTER_HPP

#include <array>
#include <vector>

#include "OBC/Components/SecureLinkAuthorizer/SecureLinkAuthorizer.hpp"
#include "OBC/Components/SecureLinkAuthorizer/SecureLinkAuthorizerGTestBase.hpp"

namespace OBC {

class SecureLinkAuthorizerTester final : public SecureLinkAuthorizerGTestBase {
  public:
    static const FwSizeType MAX_HISTORY_SIZE = 512;
    static const FwEnumStoreType TEST_INSTANCE_ID = 0;

    struct HandshakePacketCapture {
        FwIndexType portNum;
        Fw::ComBuffer packet;
    };

    struct BufferReturnCapture {
        FwIndexType portNum;
        std::vector<U8> bytes;
    };

    struct AuthGrantCapture {
        SecureAuthGrant grant;
    };

    struct AuthRevokeCapture {
        SecureAuthRevocation revocation;
    };

    SecureLinkAuthorizerTester();
    ~SecureLinkAuthorizerTester() override;

    void testReqAuthProducesChallengeOnConfiguredIngress();
    void testMatchingResponseProducesAuthGrantAndAuthenticatedStatus();
    void testBadResponseReturnsNotAuthenticatedWithoutGrant();
    void testUnsupportedServiceReturnsStatusWithoutGrant();
    void testMalformedPacketRejectsWithoutStatusOrGrant();
    void testTimeoutRevokesActiveAuth();

  private:
    void connectPorts();
    void initComponents();

    void from_bufferReturnOut_handler(FwIndexType portNum, Fw::Buffer& buffer) override;
    void from_handshakePacketOut_handler(FwIndexType portNum, Fw::ComBuffer& data, U32 context) override;
    void from_authGrantedOut_handler(FwIndexType portNum, const SecureAuthGrant& grant) override;
    void from_authRevokedOut_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) override;

    Fw::Buffer makeReqAuthPayload(U8 serviceId);
    Fw::Buffer makeResponsePayload(U8 serviceId, const U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE]);
    Fw::Buffer makeMalformedPayload();
    SecureHandshakeParseResult parseHandshakeDownlink(const HandshakePacketCapture& capture) const;
    void clearCaptures();

  private:
    SecureLinkAuthorizer component;
    std::array<U8, 32> m_sbandKey = {
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U, 0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
        0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU, 0x2FU,
    };
    std::vector<std::array<U8, 64>> m_storage;
    std::vector<HandshakePacketCapture> m_packets;
    std::vector<BufferReturnCapture> m_returns;
    std::vector<AuthGrantCapture> m_grants;
    std::vector<AuthRevokeCapture> m_revokes;
};

}  // namespace OBC

#endif
