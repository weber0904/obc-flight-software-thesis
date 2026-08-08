#include "OBC/Components/CommandIngressAuthority/SecureCommandV2Metadata.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include <Fw/Com/ComPacket.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/Serializable.hpp>

#include "OBC/Components/CommandIngressAuthority/CommandAuthCrypto.hpp"

namespace OBC {

namespace {

constexpr FwSizeType MIN_INNER_COMMAND_SIZE = sizeof(FwPacketDescriptorType) + sizeof(FwOpcodeType);
constexpr char SECURE_COMMAND_V2_AUTH_LABEL[] = "CMD-V2";

SecureCommandV2ParseResult reject(SecureCommandV2RejectReason reason) {
    SecureCommandV2ParseResult result;
    result.reason = reason;
    return result;
}

SecureCommandV2ParseResult reject(SecureCommandV2RejectReason reason, FwOpcodeType responseOpcode) {
    SecureCommandV2ParseResult result = reject(reason);
    result.responseOpcode = responseOpcode;
    return result;
}

FwOpcodeType tryDecodeInnerOpcode(const U8* data, FwSizeType size) {
    if (data == nullptr || size < MIN_INNER_COMMAND_SIZE) {
        return SECURE_COMMAND_V2_UNKNOWN_OPCODE;
    }

    Fw::ExternalSerializeBuffer buffer(const_cast<U8*>(data), size);
    if (buffer.setBuffLen(size) != Fw::FW_SERIALIZE_OK) {
        return SECURE_COMMAND_V2_UNKNOWN_OPCODE;
    }

    FwPacketDescriptorType descriptor = 0U;
    if (buffer.deserializeTo(descriptor) != Fw::FW_SERIALIZE_OK ||
        descriptor != static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)) {
        return SECURE_COMMAND_V2_UNKNOWN_OPCODE;
    }

    FwOpcodeType opcode = SECURE_COMMAND_V2_UNKNOWN_OPCODE;
    if (buffer.deserializeTo(opcode) != Fw::FW_SERIALIZE_OK) {
        return SECURE_COMMAND_V2_UNKNOWN_OPCODE;
    }
    return opcode;
}

FwOpcodeType decodeAttributableInnerOpcode(const Fw::CmdArgBuffer& args) {
    return tryDecodeInnerOpcode(args.getBuffAddrLeft(), args.getDeserializeSizeLeft());
}

bool serializeSecureCommandAuthMaterial(const SecureCommandV2Metadata& metadata,
                                        const Fw::ComBuffer& innerCommand,
                                        Fw::ComBuffer& material) {
    return material.serializeFrom(reinterpret_cast<const U8*>(SECURE_COMMAND_V2_AUTH_LABEL),
                                  sizeof(SECURE_COMMAND_V2_AUTH_LABEL) - 1U,
                                  Fw::Serialization::OMIT_LENGTH,
                                  Fw::Endianness::BIG) == Fw::FW_SERIALIZE_OK &&
           material.serializeFrom(metadata.sequenceNumber) == Fw::FW_SERIALIZE_OK &&
           material.serializeFrom(innerCommand.getBuffAddr(),
                                  innerCommand.getSize(),
                                  Fw::Serialization::OMIT_LENGTH,
                                  Fw::Endianness::BIG) == Fw::FW_SERIALIZE_OK;
}

SecureCommandV2ParseResult decodeInner(Fw::ComBuffer& innerCommand,
                                       const SecureCommandV2Metadata& metadata,
                                       FwSizeType innerLength,
                                       const U8* authTag) {
    if (innerLength > innerCommand.getCapacity()) {
        return reject(SecureCommandV2RejectReason::OVERSIZED_INNER_COMMAND);
    }
    if (innerLength < MIN_INNER_COMMAND_SIZE) {
        return reject(SecureCommandV2RejectReason::TRUNCATED_INNER_COMMAND);
    }

    Fw::CmdPacket innerPacket;
    const Fw::SerializeStatus status = innerPacket.deserializeFrom(innerCommand);
    if (status != Fw::FW_SERIALIZE_OK) {
        return reject(SecureCommandV2RejectReason::INVALID_INNER_COMMAND,
                      tryDecodeInnerOpcode(innerCommand.getBuffAddr(), innerLength));
    }

    SecureCommandV2ParseResult result;
    result.valid = true;
    result.reason = SecureCommandV2RejectReason::NONE;
    result.metadata = metadata;
    result.metadata.innerOpcode = innerPacket.getOpCode();
    result.responseOpcode = innerPacket.getOpCode();
    std::copy_n(authTag, SECURE_COMMAND_V2_MAC_LENGTH, result.authTag.begin());
    innerCommand.resetDeser();
    return result;
}

}  // namespace

SecureCommandV2ParseResult parseSecureCommandV2(Fw::CmdPacket& outerPacket, Fw::ComBuffer& innerCommand) {
    Fw::CmdArgBuffer& args = outerPacket.getArgBuffer();

    U32 magic = 0U;
    U8 version = 0U;
    U8 flags = 0U;
    U16 headerLength = 0U;
    SecureCommandV2Metadata metadata = {};
    U16 innerLength = 0U;
    U16 macLength = 0U;
    U32 reserved = 0U;

    if (args.deserializeTo(magic) != Fw::FW_SERIALIZE_OK || args.deserializeTo(version) != Fw::FW_SERIALIZE_OK ||
        args.deserializeTo(flags) != Fw::FW_SERIALIZE_OK || args.deserializeTo(headerLength) != Fw::FW_SERIALIZE_OK ||
        args.deserializeTo(metadata.sequenceNumber) != Fw::FW_SERIALIZE_OK ||
        args.deserializeTo(innerLength) != Fw::FW_SERIALIZE_OK || args.deserializeTo(macLength) != Fw::FW_SERIALIZE_OK ||
        args.deserializeTo(reserved) != Fw::FW_SERIALIZE_OK) {
        return reject(SecureCommandV2RejectReason::TRUNCATED_HEADER);
    }

    const FwOpcodeType attributableInnerOpcode = decodeAttributableInnerOpcode(args);
    if (magic != SECURE_COMMAND_V2_MAGIC) {
        return reject(SecureCommandV2RejectReason::BAD_MAGIC, attributableInnerOpcode);
    }
    if (version != SECURE_COMMAND_V2_VERSION) {
        return reject(SecureCommandV2RejectReason::UNSUPPORTED_VERSION, attributableInnerOpcode);
    }
    if (flags != 0U) {
        return reject(SecureCommandV2RejectReason::NONZERO_FLAGS, attributableInnerOpcode);
    }
    if (headerLength != SECURE_COMMAND_V2_HEADER_LENGTH) {
        return reject(SecureCommandV2RejectReason::BAD_HEADER_LENGTH, attributableInnerOpcode);
    }
    if (reserved != 0U) {
        return reject(SecureCommandV2RejectReason::NONZERO_RESERVED, attributableInnerOpcode);
    }
    if (macLength != SECURE_COMMAND_V2_MAC_LENGTH) {
        return reject(SecureCommandV2RejectReason::BAD_MAC_LENGTH, attributableInnerOpcode);
    }
    if (innerLength < MIN_INNER_COMMAND_SIZE) {
        return reject(SecureCommandV2RejectReason::TRUNCATED_INNER_COMMAND, attributableInnerOpcode);
    }

    const FwSizeType remainingBytes = args.getDeserializeSizeLeft();
    if (remainingBytes < innerLength) {
        return reject(SecureCommandV2RejectReason::TRUNCATED_INNER_COMMAND,
                      tryDecodeInnerOpcode(args.getBuffAddrLeft(), args.getDeserializeSizeLeft()));
    }
    if (remainingBytes < static_cast<FwSizeType>(innerLength + macLength)) {
        return reject(SecureCommandV2RejectReason::TRUNCATED_AUTH_TAG,
                      tryDecodeInnerOpcode(args.getBuffAddrLeft(), innerLength));
    }
    if (remainingBytes != static_cast<FwSizeType>(innerLength + macLength)) {
        return reject(SecureCommandV2RejectReason::UNEXPECTED_TRAILING_BYTES,
                      tryDecodeInnerOpcode(args.getBuffAddrLeft(), innerLength));
    }
    if (innerLength > innerCommand.getCapacity()) {
        return reject(SecureCommandV2RejectReason::OVERSIZED_INNER_COMMAND);
    }

    const U8* innerData = args.getBuffAddrLeft();
    if (innerCommand.setBuff(innerData, innerLength) != Fw::FW_SERIALIZE_OK) {
        return reject(SecureCommandV2RejectReason::OVERSIZED_INNER_COMMAND);
    }

    return decodeInner(innerCommand, metadata, innerLength, innerData + innerLength);
}

SecureCommandV2RejectReason verifySecureCommandV2Auth(
    const SecureCommandV2ParseResult& secureCommand,
    const Fw::ComBuffer& innerCommand,
    const U8 sessionKey[SECURE_COMMAND_V2_SESSION_KEY_SIZE]) {
    if (!secureCommand.valid) {
        return SecureCommandV2RejectReason::BAD_MAC;
    }
    if (sessionKey == nullptr) {
        return SecureCommandV2RejectReason::INVALID_CONFIG;
    }

    Fw::ComBuffer material;
    if (!serializeSecureCommandAuthMaterial(secureCommand.metadata, innerCommand, material)) {
        return SecureCommandV2RejectReason::INVALID_CONFIG;
    }

    U8 expectedDigest[SECURE_COMMAND_V2_MAC_LENGTH] = {};
    if (!hmacSha256(sessionKey,
                    SECURE_COMMAND_V2_SESSION_KEY_SIZE,
                    material.getBuffAddr(),
                    material.getSize(),
                    expectedDigest)) {
        return SecureCommandV2RejectReason::INVALID_CONFIG;
    }
    if (!constantTimeEqual(expectedDigest, secureCommand.authTag.data(), SECURE_COMMAND_V2_MAC_LENGTH)) {
        return SecureCommandV2RejectReason::BAD_MAC;
    }
    return SecureCommandV2RejectReason::NONE;
}

Fw::ComBuffer makeSecureCommandV2(FwOpcodeType innerOpcode,
                                  const U8 sessionKey[SECURE_COMMAND_V2_SESSION_KEY_SIZE],
                                  U32 sequenceNumber) {
    Fw::ComBuffer inner;
    static_cast<void>(inner.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)));
    static_cast<void>(inner.serializeFrom(innerOpcode));
    return makeSecureCommandV2(inner, sessionKey, sequenceNumber);
}

Fw::ComBuffer makeSecureCommandV2(const Fw::ComBuffer& innerCommand,
                                  const U8 sessionKey[SECURE_COMMAND_V2_SESSION_KEY_SIZE],
                                  U32 sequenceNumber) {
    SecureCommandV2Metadata metadata = {};
    metadata.sequenceNumber = sequenceNumber;

    Fw::ComBuffer material;
    const bool serialized = serializeSecureCommandAuthMaterial(metadata, innerCommand, material);
    FW_ASSERT(serialized);
    if (!serialized) {
        return Fw::ComBuffer();
    }
    U8 digest[SECURE_COMMAND_V2_MAC_LENGTH] = {};
    static_cast<void>(hmacSha256(sessionKey,
                                 SECURE_COMMAND_V2_SESSION_KEY_SIZE,
                                 material.getBuffAddr(),
                                 material.getSize(),
                                 digest));

    Fw::ComBuffer outer;
    static_cast<void>(outer.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)));
    static_cast<void>(outer.serializeFrom(OBC_SECURE_COMMAND_V2_OPCODE));
    static_cast<void>(outer.serializeFrom(SECURE_COMMAND_V2_MAGIC));
    static_cast<void>(outer.serializeFrom(SECURE_COMMAND_V2_VERSION));
    static_cast<void>(outer.serializeFrom(static_cast<U8>(0U)));
    static_cast<void>(outer.serializeFrom(SECURE_COMMAND_V2_HEADER_LENGTH));
    static_cast<void>(outer.serializeFrom(sequenceNumber));
    static_cast<void>(outer.serializeFrom(static_cast<U16>(innerCommand.getSize())));
    static_cast<void>(outer.serializeFrom(SECURE_COMMAND_V2_MAC_LENGTH));
    static_cast<void>(outer.serializeFrom(static_cast<U32>(0U)));
    static_cast<void>(outer.serializeFrom(innerCommand.getBuffAddr(),
                                          innerCommand.getSize(),
                                          Fw::Serialization::OMIT_LENGTH,
                                          Fw::Endianness::BIG));
    static_cast<void>(outer.serializeFrom(digest,
                                          SECURE_COMMAND_V2_MAC_LENGTH,
                                          Fw::Serialization::OMIT_LENGTH,
                                          Fw::Endianness::BIG));
    return outer;
}

}  // namespace OBC
