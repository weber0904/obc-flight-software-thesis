#ifndef OBC_SECURELINKHANDSHAKE_HPP
#define OBC_SECURELINKHANDSHAKE_HPP

#include <array>

#include "Fw/Com/ComBuffer.hpp"
#include "Fw/FPrimeBasicTypes.hpp"
#include "OBC/Components/SecureLinkAuthProtocol/FppConstantsAc.hpp"

namespace OBC {

constexpr U32 SECURE_LINK_HANDSHAKE_MAGIC = 0x0BC0A701U;
constexpr U8 SECURE_LINK_HANDSHAKE_VERSION = 1U;
constexpr U32 SECURE_LINK_HANDSHAKE_HEADER_SIZE = 8U;
constexpr U32 SECURE_LINK_MODULE_SERIAL_SIZE = 16U;
constexpr U32 SECURE_LINK_RANDOM_NONCE_SIZE = 15U;
constexpr U32 SECURE_LINK_CHALLENGE_SIZE = SECURE_LINK_MODULE_SERIAL_SIZE + SECURE_LINK_RANDOM_NONCE_SIZE;
constexpr U32 SECURE_LINK_SESSION_KEY_SIZE = 32U;
constexpr U32 SECURE_LINK_AUTH_RESPONSE_SIZE = 32U;
constexpr U32 SECURE_LINK_AUTH_TIMEOUT_SECONDS = 180U;

enum class SecureHandshakeMessageType : U8 {
    REQ_AUTH = 1U,
    CHALLENGE = 2U,
    RESPONSE = 3U,
    AUTH_STATUS = 4U,
};

enum class SecureHandshakeParseReason : U32 {
    NONE = 0U,
    TOO_SHORT = 1U,
    BAD_MAGIC = 2U,
    BAD_VERSION = 3U,
    BAD_MESSAGE_TYPE = 4U,
    BAD_SERVICE_ID = 5U,
    BAD_LENGTH = 6U,
};

struct SecureHandshakeMessage {
    SecureHandshakeMessageType type = SecureHandshakeMessageType::REQ_AUTH;
    U8 serviceId = 0U;
    std::array<U8, SECURE_LINK_CHALLENGE_SIZE> challenge = {};
    std::array<U8, SECURE_LINK_AUTH_RESPONSE_SIZE> response = {};
    U32 statusCode = 0U;
};

struct SecureHandshakeParseResult {
    bool valid = false;
    SecureHandshakeParseReason reason = SecureHandshakeParseReason::TOO_SHORT;
    SecureHandshakeMessage message = {};
};

bool isSupportedSecureServiceId(U8 serviceId);

SecureHandshakeParseResult parseSecureHandshakePacket(const U8* data, FwSizeType dataLength);

bool buildSecureChallengePacket(U8 serviceId,
                                const U8 challenge[SECURE_LINK_CHALLENGE_SIZE],
                                Fw::ComBuffer& outPacket);

bool buildSecureAuthStatusPacket(U8 serviceId, U32 statusCode, Fw::ComBuffer& outPacket);

bool deriveSecureSessionKey(const U8* rootKey,
                            FwSizeType rootKeyLength,
                            U8 serviceId,
                            const U8 challenge[SECURE_LINK_CHALLENGE_SIZE],
                            U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE]);

bool computeSecureAuthResponse(const U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE],
                               U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE]);

bool verifySecureAuthResponse(const U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE],
                              const U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE]);

}  // namespace OBC

#endif
