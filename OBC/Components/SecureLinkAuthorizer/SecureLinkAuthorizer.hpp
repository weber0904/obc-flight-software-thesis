#ifndef OBC_SECURELINKAUTHORIZER_HPP
#define OBC_SECURELINKAUTHORIZER_HPP

#include <array>

#include "Fw/Time/Time.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"
#include "OBC/Components/SecureLinkAuthorizer/SecureLinkAuthorizerComponentAc.hpp"
#include "OBC/Components/SecureLinkAuthorizer/SecureLinkHandshake.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/SecureAuthRevocationReasonEnumAc.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/SecureAuthStatusCodeEnumAc.hpp"

namespace OBC {

class SecureLinkAuthorizer final : public SecureLinkAuthorizerComponentBase {
  public:
    static constexpr FwIndexType PORT_COUNT = 2;

    explicit SecureLinkAuthorizer(const char* compName);
    ~SecureLinkAuthorizer() override;

    bool configureIngressService(FwIndexType ingressPort, U8 serviceId, const U8* keyBytes, FwSizeType keyLength);
    bool configureModuleSerial(const char* moduleSerial);

  private:
    struct IngressKeyConfig {
        bool valid = false;
        U8 serviceId = 0U;
        FwSizeType keyLength = 0U;
        std::array<U8, 64> keyBytes = {};
    };

    struct PendingChallengeState {
        bool active = false;
        U8 serviceId = 0U;
        std::array<U8, SECURE_LINK_CHALLENGE_SIZE> challenge = {};
    };

    struct ActiveAuthState {
        bool active = false;
        U8 serviceId = 0U;
        std::array<U8, SECURE_LINK_SESSION_KEY_SIZE> sessionKey = {};
        Fw::Time lastActivityTime = Fw::ZERO_TIME;
    };

  private:
    void handshakeUplinkIn_handler(FwIndexType portNum, Fw::Buffer& data, const ComCfg::FrameContext& context) override;
    void authActivityIn_handler(FwIndexType portNum, const SecureAuthActivity& activity) override;
    void authInvalidateIn_handler(FwIndexType portNum, const SecureAuthRevocation& revocation) override;
    void schedIn_handler(FwIndexType portNum, U32 context) override;

  private:
    bool issueChallenge_(FwIndexType ingressPort, U8 serviceId);
    bool establishAuth_(FwIndexType ingressPort, U8 serviceId, const U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE]);
    void clearPendingChallenge_(FwIndexType ingressPort);
    void clearActiveAuth_(FwIndexType ingressPort);
    void revokeActiveAuth_(FwIndexType ingressPort, U32 reason, bool notifyCommandGate);
    void publishActiveAuth_();
    void emitAuthStatus_(FwIndexType ingressPort, U8 serviceId, U32 statusCode);
    bool fillRandomNonce_(U8 randomNonce[SECURE_LINK_RANDOM_NONCE_SIZE]);

  private:
    std::array<U8, SECURE_LINK_MODULE_SERIAL_SIZE> m_moduleSerial = {};
    IngressKeyConfig m_ingressKeys[PORT_COUNT];
    PendingChallengeState m_pendingChallenges[PORT_COUNT];
    ActiveAuthState m_activeAuth[PORT_COUNT];
    U32 m_challengeTotal = 0U;
    U32 m_establishedTotal = 0U;
    U32 m_revokeTotal = 0U;
};

}  // namespace OBC

#endif
