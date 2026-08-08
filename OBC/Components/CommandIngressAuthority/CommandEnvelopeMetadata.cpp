#include "OBC/Components/CommandIngressAuthority/CommandEnvelopeMetadata.hpp"

#include <algorithm>
#include <array>
#include <utility>

#include <Fw/Com/ComPacket.hpp>
#include <Fw/Types/Serializable.hpp>

#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"

namespace OBC {

namespace {

constexpr FwSizeType MIN_INNER_COMMAND_SIZE = sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType);

CommandEnvelopeParseResult reject(CommandEnvelopeRejectReason reason) {
    CommandEnvelopeParseResult result;
    result.reason = reason;
    return result;
}

CommandEnvelopeParseResult reject(CommandEnvelopeRejectReason reason, FwOpcodeType responseOpcode) {
    CommandEnvelopeParseResult result = reject(reason);
    result.responseOpcode = responseOpcode;
    return result;
}

CommandEnvelopeAuthRejectReason rejectAuth(CommandEnvelopeAuthRejectReason reason) {
    return reason;
}

FwOpcodeType tryDecodeInnerOpcode(const U8* data, FwSizeType size) {
    if (size < MIN_INNER_COMMAND_SIZE) {
        return COMMAND_ENVELOPE_UNKNOWN_OPCODE;
    }

    Fw::ExternalSerializeBuffer buffer(const_cast<U8*>(data), size);
    if (buffer.setBuffLen(size) != Fw::FW_SERIALIZE_OK) {
        return COMMAND_ENVELOPE_UNKNOWN_OPCODE;
    }

    FwPacketDescriptorType descriptor = 0;
    if (buffer.deserializeTo(descriptor) != Fw::FW_SERIALIZE_OK) {
        return COMMAND_ENVELOPE_UNKNOWN_OPCODE;
    }
    if (descriptor != static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)) {
        return COMMAND_ENVELOPE_UNKNOWN_OPCODE;
    }

    FwOpcodeType opcode = COMMAND_ENVELOPE_UNKNOWN_OPCODE;
    if (buffer.deserializeTo(opcode) != Fw::FW_SERIALIZE_OK) {
        return COMMAND_ENVELOPE_UNKNOWN_OPCODE;
    }
    return opcode;
}

CommandEnvelopeParseResult decodeInner(Fw::ComBuffer& innerCommand,
                                       const CommandEnvelopeMetadata& metadata,
                                       FwSizeType innerLength,
                                       const U8* authTag) {
    CommandEnvelopeParseResult result;
    if (innerLength > innerCommand.getCapacity()) {
        return reject(CommandEnvelopeRejectReason::OVERSIZED_INNER_COMMAND);
    }
    if (innerLength < MIN_INNER_COMMAND_SIZE) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_INNER_COMMAND);
    }

    Fw::CmdPacket innerPacket;
    const Fw::SerializeStatus stat = innerPacket.deserializeFrom(innerCommand);
    if (stat != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::INVALID_INNER_COMMAND,
                      tryDecodeInnerOpcode(innerCommand.getBuffAddr(), innerLength));
    }

    result.valid = true;
    result.metadata = metadata;
    result.metadata.innerOpcode = innerPacket.getOpCode();
    result.responseOpcode = innerPacket.getOpCode();
    std::copy_n(authTag, COMMAND_ENVELOPE_V1_MAC_LENGTH, result.authTag.begin());
    innerCommand.resetDeser();
    return result;
}

FwOpcodeType decodeAttributableInnerOpcode(const Fw::CmdArgBuffer& args) {
    return tryDecodeInnerOpcode(args.getBuffAddrLeft(), args.getDeserializeSizeLeft());
}

bool serializeEnvelopeAuthMaterial(const CommandEnvelopeMetadata& metadata,
                                   const Fw::ComBuffer& innerCommand,
                                   Fw::ComBuffer& material) {
    if (material.serializeFrom(COMMAND_ENVELOPE_V1_MAGIC) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(COMMAND_ENVELOPE_V1_VERSION) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(static_cast<U8>(0U)) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(COMMAND_ENVELOPE_V1_HEADER_LENGTH) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(metadata.sourceId) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(metadata.sessionId) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(metadata.sequenceNumber) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(metadata.keySlot) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(static_cast<U16>(innerCommand.getSize())) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(COMMAND_ENVELOPE_V1_MAC_LENGTH) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    if (material.serializeFrom(static_cast<U16>(0U)) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    return material.serializeFrom(innerCommand.getBuffAddr(),
                                  innerCommand.getSize(),
                                  Fw::Serialization::OMIT_LENGTH,
                                  Fw::Endianness::BIG) == Fw::FW_SERIALIZE_OK;
}

}  // namespace

CommandEnvelopeParseResult parseCommandEnvelopeV1(Fw::CmdPacket& outerPacket, Fw::ComBuffer& innerCommand) {
    Fw::CmdArgBuffer& args = outerPacket.getArgBuffer();

    U32 magic = 0;
    U8 version = 0;
    U8 flags = 0;
    U16 headerLength = 0;
    CommandEnvelopeMetadata metadata;
    U16 keySlot = 0;
    U16 innerLength = 0;
    U16 macLength = 0;
    U16 reserved = 0;

    if (args.deserializeTo(magic) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(version) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(flags) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(headerLength) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(metadata.sourceId) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(metadata.sessionId) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(metadata.sequenceNumber) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(keySlot) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(innerLength) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(macLength) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    if (args.deserializeTo(reserved) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_HEADER);
    }
    metadata.keySlot = keySlot;

    const FwOpcodeType attributableInnerOpcode = decodeAttributableInnerOpcode(args);
    if (magic != COMMAND_ENVELOPE_V1_MAGIC) {
        return reject(CommandEnvelopeRejectReason::BAD_MAGIC, attributableInnerOpcode);
    }
    if (version != COMMAND_ENVELOPE_V1_VERSION) {
        return reject(CommandEnvelopeRejectReason::UNSUPPORTED_VERSION, attributableInnerOpcode);
    }
    if (flags != 0U) {
        return reject(CommandEnvelopeRejectReason::NONZERO_FLAGS, attributableInnerOpcode);
    }
    if (headerLength != COMMAND_ENVELOPE_V1_HEADER_LENGTH) {
        return reject(CommandEnvelopeRejectReason::BAD_HEADER_LENGTH, attributableInnerOpcode);
    }
    if (reserved != 0U) {
        return reject(CommandEnvelopeRejectReason::NONZERO_RESERVED, attributableInnerOpcode);
    }
    if (macLength != COMMAND_ENVELOPE_V1_MAC_LENGTH) {
        return reject(CommandEnvelopeRejectReason::BAD_MAC_LENGTH, attributableInnerOpcode);
    }
    if (innerLength < MIN_INNER_COMMAND_SIZE) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_INNER_COMMAND, attributableInnerOpcode);
    }
    const FwSizeType remainingBytes = args.getDeserializeSizeLeft();
    if (remainingBytes < innerLength) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_INNER_COMMAND,
                      tryDecodeInnerOpcode(args.getBuffAddrLeft(), args.getDeserializeSizeLeft()));
    }
    if (remainingBytes < static_cast<FwSizeType>(innerLength + macLength)) {
        return reject(CommandEnvelopeRejectReason::TRUNCATED_AUTH_TAG,
                      tryDecodeInnerOpcode(args.getBuffAddrLeft(), innerLength));
    }
    if (remainingBytes != static_cast<FwSizeType>(innerLength + macLength)) {
        return reject(CommandEnvelopeRejectReason::UNEXPECTED_TRAILING_BYTES,
                      tryDecodeInnerOpcode(args.getBuffAddrLeft(), innerLength));
    }
    if (innerLength > innerCommand.getCapacity()) {
        return reject(CommandEnvelopeRejectReason::OVERSIZED_INNER_COMMAND);
    }
    const U8* innerData = args.getBuffAddrLeft();
    if (innerCommand.setBuff(innerData, innerLength) != Fw::FW_SERIALIZE_OK) {
        return reject(CommandEnvelopeRejectReason::OVERSIZED_INNER_COMMAND);
    }
    const U8* authTag = innerData + innerLength;

    return decodeInner(innerCommand, metadata, innerLength, authTag);
}

CommandEnvelopeAuthRejectReason verifyCommandEnvelopeV1Auth(const CommandEnvelopeParseResult& envelope,
                                                            const Fw::ComBuffer& innerCommand,
                                                            const AuthorityConfig& config) {
    if (!envelope.valid) {
        return rejectAuth(CommandEnvelopeAuthRejectReason::BAD_MAC);
    }
    if (!config.auth.enabled || config.auth.algorithm != CommandAuthAlgorithm::HMAC_SHA256 || config.auth.keyLength == 0U) {
        return rejectAuth(CommandEnvelopeAuthRejectReason::INVALID_CONFIG);
    }
    if (config.auth.sourceId != envelope.metadata.sourceId) {
        return rejectAuth(CommandEnvelopeAuthRejectReason::SOURCE_MISMATCH);
    }
    if (config.auth.keySlot != envelope.metadata.keySlot) {
        return rejectAuth(CommandEnvelopeAuthRejectReason::UNKNOWN_KEY_SLOT);
    }

    Fw::ComBuffer material;
    if (!serializeEnvelopeAuthMaterial(envelope.metadata, innerCommand, material)) {
        return rejectAuth(CommandEnvelopeAuthRejectReason::INVALID_CONFIG);
    }

    U8 expectedDigest[COMMAND_ENVELOPE_V1_MAC_LENGTH] = {};
    if (!hmacSha256(config.auth.keyBytes,
                    static_cast<FwSizeType>(config.auth.keyLength),
                    material.getBuffAddr(),
                    material.getSize(),
                    expectedDigest)) {
        return rejectAuth(CommandEnvelopeAuthRejectReason::INVALID_CONFIG);
    }
    if (!constantTimeEqual(expectedDigest, envelope.authTag.data(), COMMAND_ENVELOPE_V1_MAC_LENGTH)) {
        return rejectAuth(CommandEnvelopeAuthRejectReason::BAD_MAC);
    }

    return CommandEnvelopeAuthRejectReason::NONE;
}

Fw::ComBuffer makeCommandEnvelopeV1(FwOpcodeType innerOpcode,
                                    U32 sourceId,
                                    U16 keySlot,
                                    const U8* keyBytes,
                                    FwSizeType keyLength,
                                    U32 sessionId,
                                    U32 sequenceNumber) {
    Fw::ComBuffer inner;
    static_cast<void>(inner.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)));
    static_cast<void>(inner.serializeFrom(innerOpcode));

    return makeCommandEnvelopeV1(inner, sourceId, keySlot, keyBytes, keyLength, sessionId, sequenceNumber);
}

Fw::ComBuffer makeCommandEnvelopeV1(const Fw::ComBuffer& inner,
                                    U32 sourceId,
                                    U16 keySlot,
                                    const U8* keyBytes,
                                    FwSizeType keyLength,
                                    U32 sessionId,
                                    U32 sequenceNumber) {

    CommandEnvelopeMetadata metadata;
    metadata.sourceId = sourceId;
    metadata.keySlot = keySlot;
    metadata.sessionId = sessionId;
    metadata.sequenceNumber = sequenceNumber;

    Fw::ComBuffer material;
    static_cast<void>(serializeEnvelopeAuthMaterial(metadata, inner, material));
    U8 digest[COMMAND_ENVELOPE_V1_MAC_LENGTH] = {};
    static_cast<void>(hmacSha256(keyBytes, keyLength, material.getBuffAddr(), material.getSize(), digest));

    Fw::ComBuffer outer;
    static_cast<void>(outer.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)));
    static_cast<void>(outer.serializeFrom(OBC_COMMAND_ENVELOPE_V1_OPCODE));
    static_cast<void>(outer.serializeFrom(COMMAND_ENVELOPE_V1_MAGIC));
    static_cast<void>(outer.serializeFrom(COMMAND_ENVELOPE_V1_VERSION));
    static_cast<void>(outer.serializeFrom(static_cast<U8>(0)));
    static_cast<void>(outer.serializeFrom(COMMAND_ENVELOPE_V1_HEADER_LENGTH));
    static_cast<void>(outer.serializeFrom(sourceId));
    static_cast<void>(outer.serializeFrom(sessionId));
    static_cast<void>(outer.serializeFrom(sequenceNumber));
    static_cast<void>(outer.serializeFrom(keySlot));
    static_cast<void>(outer.serializeFrom(static_cast<U16>(inner.getSize())));
    static_cast<void>(outer.serializeFrom(COMMAND_ENVELOPE_V1_MAC_LENGTH));
    static_cast<void>(outer.serializeFrom(static_cast<U16>(0)));
    static_cast<void>(
        outer.serializeFrom(inner.getBuffAddr(), inner.getSize(), Fw::Serialization::OMIT_LENGTH, Fw::Endianness::BIG));
    static_cast<void>(outer.serializeFrom(digest,
                                          COMMAND_ENVELOPE_V1_MAC_LENGTH,
                                          Fw::Serialization::OMIT_LENGTH,
                                          Fw::Endianness::BIG));
    return outer;
}

}  // namespace OBC
