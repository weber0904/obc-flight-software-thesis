#include "OBC/Components/SecureLinkAuthorizer/SecureLinkHandshake.hpp"

#include <array>
#include <cstring>

#include "Fw/Com/ComPacket.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"

namespace OBC {

namespace {

constexpr U8 SECURE_LINK_RESERVED = 0U;
constexpr char SECURE_LINK_DERIVATION_LABEL[] = "AUTH-SKEY-V1";
constexpr std::array<U8, SECURE_LINK_AUTH_RESPONSE_SIZE> SECURE_LINK_AKNOWN_MESSAGE = {
    0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
    0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU,
};

void storeU32(U8* dest, U32 value) {
    dest[0] = static_cast<U8>((value >> 24U) & 0xFFU);
    dest[1] = static_cast<U8>((value >> 16U) & 0xFFU);
    dest[2] = static_cast<U8>((value >> 8U) & 0xFFU);
    dest[3] = static_cast<U8>(value & 0xFFU);
}

U32 loadU32(const U8* src) {
    return (static_cast<U32>(src[0]) << 24U) | (static_cast<U32>(src[1]) << 16U) | (static_cast<U32>(src[2]) << 8U) |
           static_cast<U32>(src[3]);
}

FwSizeType payloadSizeForType(SecureHandshakeMessageType type) {
    switch (type) {
        case SecureHandshakeMessageType::REQ_AUTH:
            return SECURE_LINK_HANDSHAKE_HEADER_SIZE;
        case SecureHandshakeMessageType::CHALLENGE:
            return SECURE_LINK_HANDSHAKE_HEADER_SIZE + SECURE_LINK_CHALLENGE_SIZE;
        case SecureHandshakeMessageType::RESPONSE:
            return SECURE_LINK_HANDSHAKE_HEADER_SIZE + SECURE_LINK_AUTH_RESPONSE_SIZE;
        case SecureHandshakeMessageType::AUTH_STATUS:
            return SECURE_LINK_HANDSHAKE_HEADER_SIZE + sizeof(U32);
    }
    return 0U;
}

bool serializeHeader(Fw::ComBuffer& buffer, SecureHandshakeMessageType type, U8 serviceId) {
    return buffer.serializeFrom(SECURE_LINK_HANDSHAKE_MAGIC) == Fw::FW_SERIALIZE_OK &&
           buffer.serializeFrom(SECURE_LINK_HANDSHAKE_VERSION) == Fw::FW_SERIALIZE_OK &&
           buffer.serializeFrom(static_cast<U8>(type)) == Fw::FW_SERIALIZE_OK &&
           buffer.serializeFrom(serviceId) == Fw::FW_SERIALIZE_OK &&
           buffer.serializeFrom(SECURE_LINK_RESERVED) == Fw::FW_SERIALIZE_OK;
}

}  // namespace

bool isSupportedSecureServiceId(U8 serviceId) {
    return serviceId == OBC::SECURE_SERVICE_SBAND || serviceId == OBC::SECURE_SERVICE_UHF;
}

SecureHandshakeParseResult parseSecureHandshakePacket(const U8* data, FwSizeType dataLength) {
    SecureHandshakeParseResult result = {};
    if (data == nullptr || dataLength < SECURE_LINK_HANDSHAKE_HEADER_SIZE) {
        result.reason = SecureHandshakeParseReason::TOO_SHORT;
        return result;
    }

    const U32 magic = loadU32(data);
    if (magic != SECURE_LINK_HANDSHAKE_MAGIC) {
        result.reason = SecureHandshakeParseReason::BAD_MAGIC;
        return result;
    }
    if (data[4] != SECURE_LINK_HANDSHAKE_VERSION) {
        result.reason = SecureHandshakeParseReason::BAD_VERSION;
        return result;
    }

    const U8 rawType = data[5];
    const U8 serviceId = data[6];
    if (!isSupportedSecureServiceId(serviceId)) {
        result.reason = SecureHandshakeParseReason::BAD_SERVICE_ID;
        return result;
    }
    if (data[7] != SECURE_LINK_RESERVED) {
        result.reason = SecureHandshakeParseReason::BAD_LENGTH;
        return result;
    }

    SecureHandshakeMessageType type = SecureHandshakeMessageType::REQ_AUTH;
    switch (rawType) {
        case static_cast<U8>(SecureHandshakeMessageType::REQ_AUTH):
            type = SecureHandshakeMessageType::REQ_AUTH;
            break;
        case static_cast<U8>(SecureHandshakeMessageType::CHALLENGE):
            type = SecureHandshakeMessageType::CHALLENGE;
            break;
        case static_cast<U8>(SecureHandshakeMessageType::RESPONSE):
            type = SecureHandshakeMessageType::RESPONSE;
            break;
        case static_cast<U8>(SecureHandshakeMessageType::AUTH_STATUS):
            type = SecureHandshakeMessageType::AUTH_STATUS;
            break;
        default:
            result.reason = SecureHandshakeParseReason::BAD_MESSAGE_TYPE;
            return result;
    }

    if (dataLength != payloadSizeForType(type)) {
        result.reason = SecureHandshakeParseReason::BAD_LENGTH;
        return result;
    }

    result.message.type = type;
    result.message.serviceId = serviceId;
    if (type == SecureHandshakeMessageType::CHALLENGE) {
        std::memcpy(result.message.challenge.data(), data + SECURE_LINK_HANDSHAKE_HEADER_SIZE, SECURE_LINK_CHALLENGE_SIZE);
    } else if (type == SecureHandshakeMessageType::RESPONSE) {
        std::memcpy(result.message.response.data(),
                    data + SECURE_LINK_HANDSHAKE_HEADER_SIZE,
                    SECURE_LINK_AUTH_RESPONSE_SIZE);
    } else if (type == SecureHandshakeMessageType::AUTH_STATUS) {
        result.message.statusCode = loadU32(data + SECURE_LINK_HANDSHAKE_HEADER_SIZE);
    }

    result.valid = true;
    result.reason = SecureHandshakeParseReason::NONE;
    return result;
}

bool buildSecureChallengePacket(U8 serviceId,
                                const U8 challenge[SECURE_LINK_CHALLENGE_SIZE],
                                Fw::ComBuffer& outPacket) {
    if (!isSupportedSecureServiceId(serviceId) || challenge == nullptr) {
        return false;
    }

    outPacket.resetSer();
    if (outPacket.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_HAND)) !=
            Fw::FW_SERIALIZE_OK ||
        !serializeHeader(outPacket, SecureHandshakeMessageType::CHALLENGE, serviceId)) {
        return false;
    }
    return outPacket.serializeFrom(challenge, SECURE_LINK_CHALLENGE_SIZE, Fw::Serialization::OMIT_LENGTH) ==
           Fw::FW_SERIALIZE_OK;
}

bool buildSecureAuthStatusPacket(U8 serviceId, U32 statusCode, Fw::ComBuffer& outPacket) {
    if (!isSupportedSecureServiceId(serviceId)) {
        return false;
    }

    outPacket.resetSer();
    return outPacket.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_HAND)) ==
               Fw::FW_SERIALIZE_OK &&
           serializeHeader(outPacket, SecureHandshakeMessageType::AUTH_STATUS, serviceId) &&
           outPacket.serializeFrom(statusCode) == Fw::FW_SERIALIZE_OK;
}

bool deriveSecureSessionKey(const U8* rootKey,
                            FwSizeType rootKeyLength,
                            U8 serviceId,
                            const U8 challenge[SECURE_LINK_CHALLENGE_SIZE],
                            U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE]) {
    if (rootKey == nullptr || rootKeyLength == 0U || challenge == nullptr || sessionKey == nullptr ||
        !isSupportedSecureServiceId(serviceId)) {
        return false;
    }

    std::array<U8, sizeof(SECURE_LINK_DERIVATION_LABEL) - 1U + 1U + SECURE_LINK_CHALLENGE_SIZE> material = {};
    std::memcpy(material.data(), SECURE_LINK_DERIVATION_LABEL, sizeof(SECURE_LINK_DERIVATION_LABEL) - 1U);
    material[sizeof(SECURE_LINK_DERIVATION_LABEL) - 1U] = serviceId;
    std::memcpy(material.data() + sizeof(SECURE_LINK_DERIVATION_LABEL), challenge, SECURE_LINK_CHALLENGE_SIZE);
    return hmacSha256(rootKey, rootKeyLength, material.data(), material.size(), sessionKey);
}

bool computeSecureAuthResponse(const U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE], U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE]) {
    return sessionKey != nullptr && response != nullptr &&
           hmacSha256(sessionKey,
                      SECURE_LINK_SESSION_KEY_SIZE,
                      SECURE_LINK_AKNOWN_MESSAGE.data(),
                      SECURE_LINK_AKNOWN_MESSAGE.size(),
                      response);
}

bool verifySecureAuthResponse(const U8 sessionKey[SECURE_LINK_SESSION_KEY_SIZE],
                              const U8 response[SECURE_LINK_AUTH_RESPONSE_SIZE]) {
    if (sessionKey == nullptr || response == nullptr) {
        return false;
    }

    U8 expected[SECURE_LINK_AUTH_RESPONSE_SIZE] = {};
    if (!computeSecureAuthResponse(sessionKey, expected)) {
        return false;
    }
    return constantTimeEqual(expected, response, SECURE_LINK_AUTH_RESPONSE_SIZE);
}

}  // namespace OBC
