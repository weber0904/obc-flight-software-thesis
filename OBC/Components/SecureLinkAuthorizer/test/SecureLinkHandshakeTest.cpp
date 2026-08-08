#include <array>
#include <cstring>

#include <gtest/gtest.h>

#include "OBC/Components/SecureLinkAuthorizer/SecureLinkHandshake.hpp"

namespace {

void storeU32(U8* dest, U32 value) {
    dest[0] = static_cast<U8>((value >> 24U) & 0xFFU);
    dest[1] = static_cast<U8>((value >> 16U) & 0xFFU);
    dest[2] = static_cast<U8>((value >> 8U) & 0xFFU);
    dest[3] = static_cast<U8>(value & 0xFFU);
}

TEST(SecureLinkHandshake, ParsesChallengeAndResponseFormats) {
    std::array<U8, OBC::SECURE_LINK_HANDSHAKE_HEADER_SIZE + OBC::SECURE_LINK_CHALLENGE_SIZE> challenge = {};
    storeU32(challenge.data(), OBC::SECURE_LINK_HANDSHAKE_MAGIC);
    challenge[4] = OBC::SECURE_LINK_HANDSHAKE_VERSION;
    challenge[5] = static_cast<U8>(OBC::SecureHandshakeMessageType::CHALLENGE);
    challenge[6] = OBC::SECURE_SERVICE_SBAND;
    for (FwSizeType i = 0; i < OBC::SECURE_LINK_CHALLENGE_SIZE; i++) {
        challenge[OBC::SECURE_LINK_HANDSHAKE_HEADER_SIZE + i] = static_cast<U8>(i + 1U);
    }

    const OBC::SecureHandshakeParseResult challengeResult =
        OBC::parseSecureHandshakePacket(challenge.data(), challenge.size());
    ASSERT_TRUE(challengeResult.valid);
    EXPECT_EQ(challengeResult.message.type, OBC::SecureHandshakeMessageType::CHALLENGE);
    EXPECT_EQ(challengeResult.message.serviceId, OBC::SECURE_SERVICE_SBAND);
    EXPECT_EQ(challengeResult.message.challenge[0], 1U);

    std::array<U8, OBC::SECURE_LINK_HANDSHAKE_HEADER_SIZE + OBC::SECURE_LINK_AUTH_RESPONSE_SIZE> response = {};
    storeU32(response.data(), OBC::SECURE_LINK_HANDSHAKE_MAGIC);
    response[4] = OBC::SECURE_LINK_HANDSHAKE_VERSION;
    response[5] = static_cast<U8>(OBC::SecureHandshakeMessageType::RESPONSE);
    response[6] = OBC::SECURE_SERVICE_UHF;
    response[7] = 0U;
    std::memset(response.data() + OBC::SECURE_LINK_HANDSHAKE_HEADER_SIZE, 0xAA, OBC::SECURE_LINK_AUTH_RESPONSE_SIZE);

    const OBC::SecureHandshakeParseResult responseResult =
        OBC::parseSecureHandshakePacket(response.data(), response.size());
    ASSERT_TRUE(responseResult.valid);
    EXPECT_EQ(responseResult.message.type, OBC::SecureHandshakeMessageType::RESPONSE);
    EXPECT_EQ(responseResult.message.serviceId, OBC::SECURE_SERVICE_UHF);
    EXPECT_EQ(responseResult.message.response[0], 0xAAU);
}

TEST(SecureLinkHandshake, DerivesSessionKeyAndVerifiesResponse) {
    const std::array<U8, 32> rootKey = {
        0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U, 0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
        0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU, 0x2FU,
    };
    const std::array<U8, OBC::SECURE_LINK_CHALLENGE_SIZE> challenge = {
        'F', 'P', 'O', 'B', 'C', 'S', 'A', 'T', '0', '0', '0', '0', '0', '0', '0', '1',
        0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U, 0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU,
    };
    U8 sessionKey[OBC::SECURE_LINK_SESSION_KEY_SIZE] = {};
    U8 response[OBC::SECURE_LINK_AUTH_RESPONSE_SIZE] = {};
    ASSERT_TRUE(OBC::deriveSecureSessionKey(rootKey.data(), rootKey.size(), OBC::SECURE_SERVICE_SBAND, challenge.data(), sessionKey));
    ASSERT_TRUE(OBC::computeSecureAuthResponse(sessionKey, response));
    EXPECT_TRUE(OBC::verifySecureAuthResponse(sessionKey, response));
    response[0] ^= 0x55U;
    EXPECT_FALSE(OBC::verifySecureAuthResponse(sessionKey, response));
}

}  // namespace
