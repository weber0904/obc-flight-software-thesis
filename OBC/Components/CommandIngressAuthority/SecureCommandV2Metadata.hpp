#ifndef OBC_SecureCommandV2Metadata_HPP
#define OBC_SecureCommandV2Metadata_HPP

#include <array>

#include <Fw/Cmd/CmdPacket.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/FPrimeBasicTypes.hpp>

namespace OBC {

constexpr FwOpcodeType OBC_SECURE_COMMAND_V2_OPCODE = 0x0BC20001U;
constexpr U32 SECURE_COMMAND_V2_MAGIC = 0x0BC0DE02U;
constexpr U8 SECURE_COMMAND_V2_VERSION = 2U;
constexpr U16 SECURE_COMMAND_V2_HEADER_LENGTH = 20U;
constexpr U16 SECURE_COMMAND_V2_MAC_LENGTH = 32U;
constexpr FwOpcodeType SECURE_COMMAND_V2_UNKNOWN_OPCODE = 0xFFFFFFFFU;
constexpr FwSizeType SECURE_COMMAND_V2_SESSION_KEY_SIZE = 32U;

enum class SecureCommandV2RejectReason : U32 {
    NONE = 0U,
    BAD_MAGIC = 1U,
    UNSUPPORTED_VERSION = 2U,
    NONZERO_FLAGS = 3U,
    BAD_HEADER_LENGTH = 4U,
    NONZERO_RESERVED = 5U,
    TRUNCATED_HEADER = 6U,
    TRUNCATED_INNER_COMMAND = 7U,
    OVERSIZED_INNER_COMMAND = 8U,
    BAD_MAC_LENGTH = 9U,
    TRUNCATED_AUTH_TAG = 10U,
    UNEXPECTED_TRAILING_BYTES = 11U,
    INVALID_INNER_COMMAND = 12U,
    AUTH_REQUIRED = 13U,
    SERVICE_MISMATCH = 14U,
    BAD_MAC = 15U,
    SEQUENCE_NOT_INCREASING = 16U,
    SEQUENCE_WINDOW_FULL = 17U,
    INVALID_CONFIG = 18U,
};

struct SecureCommandV2Metadata {
    U32 sequenceNumber = 0U;
    FwOpcodeType innerOpcode = SECURE_COMMAND_V2_UNKNOWN_OPCODE;
};

struct SecureCommandV2ParseResult {
    bool valid = false;
    SecureCommandV2RejectReason reason = SecureCommandV2RejectReason::NONE;
    SecureCommandV2Metadata metadata = {};
    FwOpcodeType responseOpcode = SECURE_COMMAND_V2_UNKNOWN_OPCODE;
    std::array<U8, SECURE_COMMAND_V2_MAC_LENGTH> authTag = {};
};

SecureCommandV2ParseResult parseSecureCommandV2(Fw::CmdPacket& outerPacket, Fw::ComBuffer& innerCommand);

SecureCommandV2RejectReason verifySecureCommandV2Auth(
    const SecureCommandV2ParseResult& secureCommand,
    const Fw::ComBuffer& innerCommand,
    const U8 sessionKey[SECURE_COMMAND_V2_SESSION_KEY_SIZE]);

Fw::ComBuffer makeSecureCommandV2(FwOpcodeType innerOpcode,
                                  const U8 sessionKey[SECURE_COMMAND_V2_SESSION_KEY_SIZE],
                                  U32 sequenceNumber);

Fw::ComBuffer makeSecureCommandV2(const Fw::ComBuffer& innerCommand,
                                  const U8 sessionKey[SECURE_COMMAND_V2_SESSION_KEY_SIZE],
                                  U32 sequenceNumber);

}  // namespace OBC

#endif
