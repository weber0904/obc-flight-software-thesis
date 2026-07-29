#include <array>

#include <gtest/gtest.h>

#include "OBC/Components/CommandIngressAuthority/CommandEnvelopeMetadata.hpp"

namespace {

constexpr FwOpcodeType OPCODE_MODE_SET = 268632064U;
constexpr U32 SOURCE_ID = 1U;
constexpr U16 KEY_SLOT = 1U;
constexpr U32 SESSION_ID = 42U;
constexpr U32 SEQUENCE_NUMBER = 7U;
constexpr U8 TEST_KEY_BYTES[] = {
    0x10U, 0x11U, 0x12U, 0x13U, 0x14U, 0x15U, 0x16U, 0x17U, 0x18U, 0x19U, 0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU,
    0x20U, 0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U, 0x27U, 0x28U, 0x29U, 0x2AU, 0x2BU, 0x2CU, 0x2DU, 0x2EU, 0x2FU,
};

Fw::CmdPacket outerPacketFrom(Fw::ComBuffer& buffer) {
    Fw::CmdPacket packet;
    EXPECT_EQ(packet.deserializeFrom(buffer), Fw::FW_SERIALIZE_OK);
    return packet;
}

Fw::ComBuffer makeInnerCommand(FwOpcodeType opcode) {
    Fw::ComBuffer inner;
    EXPECT_EQ(inner.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)),
              Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(inner.serializeFrom(opcode), Fw::FW_SERIALIZE_OK);
    return inner;
}

std::array<U8, OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH> authTagFilled(U8 value) {
    std::array<U8, OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH> authTag = {};
    authTag.fill(value);
    return authTag;
}

Fw::ComBuffer makeOuterEnvelopeWithHeader(U32 magic,
                                          U8 version,
                                          U8 flags,
                                          U16 headerLength,
                                          U32 sourceId,
                                          U32 sessionId,
                                          U32 sequenceNumber,
                                          U16 keySlot,
                                          U16 innerLength,
                                          U16 macLength,
                                          U16 reserved,
                                          const Fw::ComBuffer& inner,
                                          const std::array<U8, OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH>& authTag,
                                          FwSizeType authTagSize = OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                          const U8* trailing = nullptr,
                                          FwSizeType trailingSize = 0U) {
    Fw::ComBuffer outer;
    EXPECT_EQ(outer.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)),
              Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(OBC::OBC_COMMAND_ENVELOPE_V1_OPCODE), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(magic), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(version), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(flags), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(headerLength), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(sourceId), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(sessionId), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(sequenceNumber), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(keySlot), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(innerLength), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(macLength), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(reserved), Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(inner.getBuffAddr(),
                                  inner.getSize(),
                                  Fw::Serialization::OMIT_LENGTH,
                                  Fw::Endianness::BIG),
              Fw::FW_SERIALIZE_OK);
    EXPECT_EQ(outer.serializeFrom(authTag.data(),
                                  authTagSize,
                                  Fw::Serialization::OMIT_LENGTH,
                                  Fw::Endianness::BIG),
              Fw::FW_SERIALIZE_OK);
    if (trailing != nullptr && trailingSize > 0U) {
        EXPECT_EQ(outer.serializeFrom(trailing, trailingSize, Fw::Serialization::OMIT_LENGTH, Fw::Endianness::BIG),
                  Fw::FW_SERIALIZE_OK);
    }
    return outer;
}

OBC::CommandEnvelopeParseResult parse(Fw::ComBuffer& outer, Fw::ComBuffer& inner) {
    Fw::CmdPacket packet = outerPacketFrom(outer);
    return OBC::parseCommandEnvelopeV1(packet, inner);
}

}  // namespace

TEST(CommandEnvelopeMetadata, ParsesValidEnvelope) {
    Fw::ComBuffer outer = OBC::makeCommandEnvelopeV1(OPCODE_MODE_SET,
                                                     SOURCE_ID,
                                                     KEY_SLOT,
                                                     TEST_KEY_BYTES,
                                                     FW_NUM_ARRAY_ELEMENTS(TEST_KEY_BYTES),
                                                     SESSION_ID,
                                                     SEQUENCE_NUMBER);
    Fw::ComBuffer inner;

    const OBC::CommandEnvelopeParseResult result = parse(outer, inner);

    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.reason, OBC::CommandEnvelopeRejectReason::NONE);
    EXPECT_EQ(result.metadata.sourceId, SOURCE_ID);
    EXPECT_EQ(result.metadata.keySlot, KEY_SLOT);
    EXPECT_EQ(result.metadata.sessionId, SESSION_ID);
    EXPECT_EQ(result.metadata.sequenceNumber, SEQUENCE_NUMBER);
    EXPECT_EQ(result.metadata.innerOpcode, OPCODE_MODE_SET);
    EXPECT_EQ(result.responseOpcode, OPCODE_MODE_SET);
}

TEST(CommandEnvelopeMetadata, RejectsBadMagic) {
    Fw::ComBuffer inner = makeInnerCommand(OPCODE_MODE_SET);
    Fw::ComBuffer outer = makeOuterEnvelopeWithHeader(0U,
                                                      OBC::COMMAND_ENVELOPE_V1_VERSION,
                                                      0U,
                                                      OBC::COMMAND_ENVELOPE_V1_HEADER_LENGTH,
                                                      SOURCE_ID,
                                                      SESSION_ID,
                                                      SEQUENCE_NUMBER,
                                                      KEY_SLOT,
                                                      static_cast<U16>(inner.getSize()),
                                                      OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                      0U,
                                                      inner,
                                                      authTagFilled(0xAAU));
    Fw::ComBuffer parsedInner;

    const OBC::CommandEnvelopeParseResult result = parse(outer, parsedInner);

    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.reason, OBC::CommandEnvelopeRejectReason::BAD_MAGIC);
    EXPECT_EQ(result.responseOpcode, OPCODE_MODE_SET);
}

TEST(CommandEnvelopeMetadata, RejectsUnsupportedVersionFlagsHeaderReservedAndMacLength) {
    Fw::ComBuffer inner = makeInnerCommand(OPCODE_MODE_SET);
    const U16 innerSize = static_cast<U16>(inner.getSize());
    Fw::ComBuffer parsedInner;

    Fw::ComBuffer badVersion = makeOuterEnvelopeWithHeader(OBC::COMMAND_ENVELOPE_V1_MAGIC,
                                                           2U,
                                                           0U,
                                                           OBC::COMMAND_ENVELOPE_V1_HEADER_LENGTH,
                                                           SOURCE_ID,
                                                           SESSION_ID,
                                                           SEQUENCE_NUMBER,
                                                           KEY_SLOT,
                                                           innerSize,
                                                           OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                           0U,
                                                           inner,
                                                           authTagFilled(0xA1U));
    EXPECT_EQ(parse(badVersion, parsedInner).reason, OBC::CommandEnvelopeRejectReason::UNSUPPORTED_VERSION);

    Fw::ComBuffer badFlags = makeOuterEnvelopeWithHeader(OBC::COMMAND_ENVELOPE_V1_MAGIC,
                                                         OBC::COMMAND_ENVELOPE_V1_VERSION,
                                                         1U,
                                                         OBC::COMMAND_ENVELOPE_V1_HEADER_LENGTH,
                                                         SOURCE_ID,
                                                         SESSION_ID,
                                                         SEQUENCE_NUMBER,
                                                         KEY_SLOT,
                                                         innerSize,
                                                         OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                         0U,
                                                         inner,
                                                         authTagFilled(0xA2U));
    EXPECT_EQ(parse(badFlags, parsedInner).reason, OBC::CommandEnvelopeRejectReason::NONZERO_FLAGS);

    Fw::ComBuffer badHeader = makeOuterEnvelopeWithHeader(OBC::COMMAND_ENVELOPE_V1_MAGIC,
                                                          OBC::COMMAND_ENVELOPE_V1_VERSION,
                                                          0U,
                                                          20U,
                                                          SOURCE_ID,
                                                          SESSION_ID,
                                                          SEQUENCE_NUMBER,
                                                          KEY_SLOT,
                                                          innerSize,
                                                          OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                          0U,
                                                          inner,
                                                          authTagFilled(0xA3U));
    EXPECT_EQ(parse(badHeader, parsedInner).reason, OBC::CommandEnvelopeRejectReason::BAD_HEADER_LENGTH);

    Fw::ComBuffer badReserved = makeOuterEnvelopeWithHeader(OBC::COMMAND_ENVELOPE_V1_MAGIC,
                                                            OBC::COMMAND_ENVELOPE_V1_VERSION,
                                                            0U,
                                                            OBC::COMMAND_ENVELOPE_V1_HEADER_LENGTH,
                                                            SOURCE_ID,
                                                            SESSION_ID,
                                                            SEQUENCE_NUMBER,
                                                            KEY_SLOT,
                                                            innerSize,
                                                            OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                            1U,
                                                            inner,
                                                            authTagFilled(0xA4U));
    EXPECT_EQ(parse(badReserved, parsedInner).reason, OBC::CommandEnvelopeRejectReason::NONZERO_RESERVED);

    Fw::ComBuffer badMacLength = makeOuterEnvelopeWithHeader(OBC::COMMAND_ENVELOPE_V1_MAGIC,
                                                             OBC::COMMAND_ENVELOPE_V1_VERSION,
                                                             0U,
                                                             OBC::COMMAND_ENVELOPE_V1_HEADER_LENGTH,
                                                             SOURCE_ID,
                                                             SESSION_ID,
                                                             SEQUENCE_NUMBER,
                                                             KEY_SLOT,
                                                             innerSize,
                                                             31U,
                                                             0U,
                                                             inner,
                                                             authTagFilled(0xA5U));
    EXPECT_EQ(parse(badMacLength, parsedInner).reason, OBC::CommandEnvelopeRejectReason::BAD_MAC_LENGTH);
}

TEST(CommandEnvelopeMetadata, RejectsInnerAndAuthTruncationAndTrailingBytes) {
    Fw::ComBuffer inner = makeInnerCommand(OPCODE_MODE_SET);
    Fw::ComBuffer parsedInner;

    Fw::ComBuffer tooSmallInner = makeOuterEnvelopeWithHeader(OBC::COMMAND_ENVELOPE_V1_MAGIC,
                                                              OBC::COMMAND_ENVELOPE_V1_VERSION,
                                                              0U,
                                                              OBC::COMMAND_ENVELOPE_V1_HEADER_LENGTH,
                                                              SOURCE_ID,
                                                              SESSION_ID,
                                                              SEQUENCE_NUMBER,
                                                              KEY_SLOT,
                                                              1U,
                                                              OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                              0U,
                                                              inner,
                                                              authTagFilled(0xB1U));
    EXPECT_EQ(parse(tooSmallInner, parsedInner).reason, OBC::CommandEnvelopeRejectReason::TRUNCATED_INNER_COMMAND);

    Fw::ComBuffer truncatedAuth = makeOuterEnvelopeWithHeader(OBC::COMMAND_ENVELOPE_V1_MAGIC,
                                                              OBC::COMMAND_ENVELOPE_V1_VERSION,
                                                              0U,
                                                              OBC::COMMAND_ENVELOPE_V1_HEADER_LENGTH,
                                                              SOURCE_ID,
                                                              SESSION_ID,
                                                              SEQUENCE_NUMBER,
                                                              KEY_SLOT,
                                                              static_cast<U16>(inner.getSize()),
                                                              OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                              0U,
                                                              inner,
                                                              authTagFilled(0xB2U),
                                                              OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH - 1U);
    EXPECT_EQ(parse(truncatedAuth, parsedInner).reason, OBC::CommandEnvelopeRejectReason::TRUNCATED_AUTH_TAG);

    const U8 trailingBytes[] = {0x55U};
    Fw::ComBuffer trailing = makeOuterEnvelopeWithHeader(OBC::COMMAND_ENVELOPE_V1_MAGIC,
                                                         OBC::COMMAND_ENVELOPE_V1_VERSION,
                                                         0U,
                                                         OBC::COMMAND_ENVELOPE_V1_HEADER_LENGTH,
                                                         SOURCE_ID,
                                                         SESSION_ID,
                                                         SEQUENCE_NUMBER,
                                                         KEY_SLOT,
                                                         static_cast<U16>(inner.getSize()),
                                                         OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                         0U,
                                                         inner,
                                                         authTagFilled(0xB3U),
                                                         OBC::COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                                         trailingBytes,
                                                         sizeof(trailingBytes));
    EXPECT_EQ(parse(trailing, parsedInner).reason, OBC::CommandEnvelopeRejectReason::UNEXPECTED_TRAILING_BYTES);
}

TEST(CommandEnvelopeMetadata, SequenceValuesAreOnlyParsed) {
    Fw::ComBuffer first = OBC::makeCommandEnvelopeV1(OPCODE_MODE_SET,
                                                     SOURCE_ID,
                                                     KEY_SLOT,
                                                     TEST_KEY_BYTES,
                                                     FW_NUM_ARRAY_ELEMENTS(TEST_KEY_BYTES),
                                                     SESSION_ID,
                                                     5U);
    Fw::ComBuffer duplicate = OBC::makeCommandEnvelopeV1(OPCODE_MODE_SET,
                                                         SOURCE_ID,
                                                         KEY_SLOT,
                                                         TEST_KEY_BYTES,
                                                         FW_NUM_ARRAY_ELEMENTS(TEST_KEY_BYTES),
                                                         SESSION_ID,
                                                         5U);
    Fw::ComBuffer lower = OBC::makeCommandEnvelopeV1(OPCODE_MODE_SET,
                                                     SOURCE_ID,
                                                     KEY_SLOT,
                                                     TEST_KEY_BYTES,
                                                     FW_NUM_ARRAY_ELEMENTS(TEST_KEY_BYTES),
                                                     SESSION_ID,
                                                     4U);
    Fw::ComBuffer parsedInner;

    EXPECT_TRUE(parse(first, parsedInner).valid);
    EXPECT_TRUE(parse(duplicate, parsedInner).valid);
    EXPECT_TRUE(parse(lower, parsedInner).valid);
}
