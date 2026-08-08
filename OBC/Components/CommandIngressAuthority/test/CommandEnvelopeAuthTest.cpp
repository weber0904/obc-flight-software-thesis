#include <array>
#include <cstring>
#include <limits>
#include <string>

#include <gtest/gtest.h>

#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"
#include "OBC/Components/CommandIngressAuthority/CommandEnvelopeMetadata.hpp"

namespace {

constexpr FwOpcodeType OPCODE_MODE_SET = 268632064U;
constexpr FwOpcodeType OPCODE_MODE_GET = 268632065U;
constexpr U32 SBAND_SOURCE_ID = 1U;
constexpr U16 SBAND_KEY_SLOT = 1U;
constexpr U32 UHF_SOURCE_ID = 2U;
constexpr U16 UHF_KEY_SLOT = 2U;
constexpr U32 SESSION_ID = 77U;
constexpr U32 SEQUENCE_NUMBER = 3U;
constexpr U8 SBAND_KEY_BYTES[] = {
    0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U, 0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
    0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU, 0x2FU,
};

void expectDigestHex(const U8 digest[OBC::COMMAND_AUTH_SHA256_DIGEST_SIZE], const char* expectedHex) {
    static constexpr char HEX[] = "0123456789abcdef";
    std::string actual;
    actual.reserve(OBC::COMMAND_AUTH_SHA256_DIGEST_SIZE * 2U);
    for (FwSizeType i = 0; i < OBC::COMMAND_AUTH_SHA256_DIGEST_SIZE; i++) {
        actual.push_back(HEX[(digest[i] >> 4U) & 0x0FU]);
        actual.push_back(HEX[digest[i] & 0x0FU]);
    }
    EXPECT_EQ(actual, expectedHex);
}

Fw::CmdPacket outerPacketFrom(Fw::ComBuffer& buffer) {
    Fw::CmdPacket packet;
    EXPECT_EQ(packet.deserializeFrom(buffer), Fw::FW_SERIALIZE_OK);
    return packet;
}

OBC::AuthorityConfig sbandAuthorityWithAuth() {
    OBC::AuthorityConfig config = OBC::authorityConfigFromProfile("sband-primary");
    EXPECT_TRUE(OBC::configureAuthorityAuth(
        config, SBAND_SOURCE_ID, SBAND_KEY_SLOT, SBAND_KEY_BYTES, FW_NUM_ARRAY_ELEMENTS(SBAND_KEY_BYTES)));
    return config;
}

OBC::CommandEnvelopeParseResult parseEnvelope(Fw::ComBuffer& outer, Fw::ComBuffer& inner) {
    Fw::CmdPacket packet = outerPacketFrom(outer);
    return OBC::parseCommandEnvelopeV1(packet, inner);
}

Fw::ComBuffer makeInnerCommand(FwOpcodeType opcode) {
    Fw::ComBuffer inner;
    EXPECT_EQ(inner.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)),
              Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(inner.serializeFrom(opcode), Fw::FW_SERIALIZE_OK);
    return inner;
}

}  // namespace

TEST(CommandEnvelopeAuth, AcceptsMatchingSourceKeyAndPayload) {
    OBC::AuthorityConfig config = sbandAuthorityWithAuth();
    Fw::ComBuffer outer = OBC::makeCommandEnvelopeV1(OPCODE_MODE_SET,
                                                     SBAND_SOURCE_ID,
                                                     SBAND_KEY_SLOT,
                                                     SBAND_KEY_BYTES,
                                                     FW_NUM_ARRAY_ELEMENTS(SBAND_KEY_BYTES),
                                                     SESSION_ID,
                                                     SEQUENCE_NUMBER);
    Fw::ComBuffer inner;

    const OBC::CommandEnvelopeParseResult envelope = parseEnvelope(outer, inner);

    EXPECT_EQ(OBC::verifyCommandEnvelopeV1Auth(envelope, inner, config), OBC::CommandEnvelopeAuthRejectReason::NONE);
}

TEST(CommandEnvelopeAuth, RejectsPayloadTamperAsBadMac) {
    OBC::AuthorityConfig config = sbandAuthorityWithAuth();
    Fw::ComBuffer outer = OBC::makeCommandEnvelopeV1(OPCODE_MODE_SET,
                                                     SBAND_SOURCE_ID,
                                                     SBAND_KEY_SLOT,
                                                     SBAND_KEY_BYTES,
                                                     FW_NUM_ARRAY_ELEMENTS(SBAND_KEY_BYTES),
                                                     SESSION_ID,
                                                     SEQUENCE_NUMBER);
    Fw::ComBuffer parsedInner;
    const OBC::CommandEnvelopeParseResult envelope = parseEnvelope(outer, parsedInner);
    Fw::ComBuffer tamperedInner = makeInnerCommand(OPCODE_MODE_GET);

    EXPECT_EQ(OBC::verifyCommandEnvelopeV1Auth(envelope, tamperedInner, config),
              OBC::CommandEnvelopeAuthRejectReason::BAD_MAC);
}

TEST(CommandEnvelopeAuth, RejectsSourceMismatchAndUnknownKeySlot) {
    OBC::AuthorityConfig config = sbandAuthorityWithAuth();
    Fw::ComBuffer outer = OBC::makeCommandEnvelopeV1(OPCODE_MODE_SET,
                                                     SBAND_SOURCE_ID,
                                                     SBAND_KEY_SLOT,
                                                     SBAND_KEY_BYTES,
                                                     FW_NUM_ARRAY_ELEMENTS(SBAND_KEY_BYTES),
                                                     SESSION_ID,
                                                     SEQUENCE_NUMBER);
    Fw::ComBuffer inner;
    const OBC::CommandEnvelopeParseResult envelope = parseEnvelope(outer, inner);

    OBC::AuthorityConfig sourceMismatch = config;
    sourceMismatch.auth.sourceId = UHF_SOURCE_ID;
    EXPECT_EQ(OBC::verifyCommandEnvelopeV1Auth(envelope, inner, sourceMismatch),
              OBC::CommandEnvelopeAuthRejectReason::SOURCE_MISMATCH);

    OBC::AuthorityConfig keySlotMismatch = config;
    keySlotMismatch.auth.keySlot = UHF_KEY_SLOT;
    EXPECT_EQ(OBC::verifyCommandEnvelopeV1Auth(envelope, inner, keySlotMismatch),
              OBC::CommandEnvelopeAuthRejectReason::UNKNOWN_KEY_SLOT);
}

TEST(CommandEnvelopeAuth, RejectsMissingOrDisabledAuthConfig) {
    OBC::AuthorityConfig config = OBC::authorityConfigFromProfile("sband-primary");
    Fw::ComBuffer outer = OBC::makeCommandEnvelopeV1(OPCODE_MODE_SET,
                                                     SBAND_SOURCE_ID,
                                                     SBAND_KEY_SLOT,
                                                     SBAND_KEY_BYTES,
                                                     FW_NUM_ARRAY_ELEMENTS(SBAND_KEY_BYTES),
                                                     SESSION_ID,
                                                     SEQUENCE_NUMBER);
    Fw::ComBuffer inner;
    const OBC::CommandEnvelopeParseResult envelope = parseEnvelope(outer, inner);

    EXPECT_EQ(OBC::verifyCommandEnvelopeV1Auth(envelope, inner, config),
              OBC::CommandEnvelopeAuthRejectReason::INVALID_CONFIG);
}

TEST(CommandEnvelopeAuth, EnforcesUnsignedSizeContractAndRejectsZeroLengthKey) {
    EXPECT_FALSE(std::numeric_limits<FwSizeType>::is_signed);

    OBC::AuthorityConfig config = OBC::authorityConfigFromProfile("sband-primary");
    EXPECT_FALSE(OBC::configureAuthorityAuth(config,
                                             SBAND_SOURCE_ID,
                                             SBAND_KEY_SLOT,
                                             SBAND_KEY_BYTES,
                                             static_cast<FwSizeType>(0U)));
    EXPECT_FALSE(config.auth.enabled);

    U8 digest[OBC::COMMAND_AUTH_SHA256_DIGEST_SIZE] = {};
    const U8 payload[] = {0xAAU, 0x55U};
    EXPECT_FALSE(OBC::hmacSha256(SBAND_KEY_BYTES,
                                 static_cast<FwSizeType>(0U),
                                 payload,
                                 static_cast<FwSizeType>(FW_NUM_ARRAY_ELEMENTS(payload)),
                                 digest));
    EXPECT_TRUE(OBC::constantTimeEqual(payload, payload, static_cast<FwSizeType>(0U)));
}

TEST(CommandEnvelopeAuth, HmacSha256MatchesRfc4231KnownAnswerVectors) {
    {
        const std::array<U8, 20> key = {
            0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU,
            0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU, 0x0bU,
        };
        const char* data = "Hi There";
        U8 digest[OBC::COMMAND_AUTH_SHA256_DIGEST_SIZE] = {};
        ASSERT_TRUE(OBC::hmacSha256(key.data(),
                                    key.size(),
                                    reinterpret_cast<const U8*>(data),
                                    std::strlen(data),
                                    digest));
        expectDigestHex(digest, "b0344c61d8db38535ca8afceaf0bf12b"
                                "881dc200c9833da726e9376c2e32cff7");
    }
    {
        const char* key = "Jefe";
        const char* data = "what do ya want for nothing?";
        U8 digest[OBC::COMMAND_AUTH_SHA256_DIGEST_SIZE] = {};
        ASSERT_TRUE(OBC::hmacSha256(reinterpret_cast<const U8*>(key),
                                    std::strlen(key),
                                    reinterpret_cast<const U8*>(data),
                                    std::strlen(data),
                                    digest));
        expectDigestHex(digest, "5bdcc146bf60754e6a042426089575c7"
                                "5a003f089d2739839dec58b964ec3843");
    }
    {
        const std::array<U8, 20> key = {
            0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU,
            0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU, 0xaaU,
        };
        const std::array<U8, 50> data = {
            0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU,
            0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU,
            0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU,
            0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU,
            0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU, 0xddU,
        };
        U8 digest[OBC::COMMAND_AUTH_SHA256_DIGEST_SIZE] = {};
        ASSERT_TRUE(OBC::hmacSha256(key.data(), key.size(), data.data(), data.size(), digest));
        expectDigestHex(digest, "773ea91e36800e46854db8ebd09181a7"
                                "2959098b3ef8c122d9635514ced565fe");
    }
}
