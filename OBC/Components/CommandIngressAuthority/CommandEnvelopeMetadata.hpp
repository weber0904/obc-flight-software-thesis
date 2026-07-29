#ifndef OBC_CommandEnvelopeMetadata_HPP
#define OBC_CommandEnvelopeMetadata_HPP

#include <array>

#include <Fw/Cmd/CmdPacket.hpp>
#include <Fw/Com/ComBuffer.hpp>
#include <Fw/FPrimeBasicTypes.hpp>

#include "OBC/Components/CommandIngressAuthority/CommandAuthorityTypes.hpp"

namespace OBC {

constexpr FwOpcodeType OBC_COMMAND_ENVELOPE_V1_OPCODE = 0x0BC10001U;
constexpr U32 COMMAND_ENVELOPE_V1_MAGIC = 0x0BC0DE01U;
constexpr U8 COMMAND_ENVELOPE_V1_VERSION = 1U;
constexpr U16 COMMAND_ENVELOPE_V1_HEADER_LENGTH = 28U;
constexpr U16 COMMAND_ENVELOPE_V1_MAC_LENGTH = 32U;
constexpr FwOpcodeType COMMAND_ENVELOPE_UNKNOWN_OPCODE = 0xFFFFFFFFU;

enum class CommandEnvelopeRejectReason : U32 {
    NONE = 0,
    BAD_MAGIC = 1,
    UNSUPPORTED_VERSION = 2,
    NONZERO_FLAGS = 3,
    BAD_HEADER_LENGTH = 4,
    NONZERO_RESERVED = 5,
    TRUNCATED_HEADER = 6,
    TRUNCATED_INNER_COMMAND = 7,
    OVERSIZED_INNER_COMMAND = 8,
    BAD_MAC_LENGTH = 9,
    TRUNCATED_AUTH_TAG = 10,
    UNEXPECTED_TRAILING_BYTES = 11,
    INVALID_INNER_COMMAND = 12,
    LEGACY_UNSUPPORTED = 13,
};

enum class CommandEnvelopeAuthRejectReason : U32 {
    NONE = 0,
    INVALID_CONFIG = 1,
    SOURCE_MISMATCH = 2,
    UNKNOWN_KEY_SLOT = 3,
    BAD_MAC = 4,
};

struct CommandEnvelopeMetadata {
    U32 sourceId = 0;
    U16 keySlot = 0;
    U32 sessionId = 0;
    U32 sequenceNumber = 0;
    FwOpcodeType innerOpcode = COMMAND_ENVELOPE_UNKNOWN_OPCODE;
};

struct CommandEnvelopeParseResult {
    bool valid = false;
    CommandEnvelopeRejectReason reason = CommandEnvelopeRejectReason::NONE;
    CommandEnvelopeMetadata metadata;
    FwOpcodeType responseOpcode = COMMAND_ENVELOPE_UNKNOWN_OPCODE;
    std::array<U8, COMMAND_ENVELOPE_V1_MAC_LENGTH> authTag = {};
};

CommandEnvelopeParseResult parseCommandEnvelopeV1(Fw::CmdPacket& outerPacket, Fw::ComBuffer& innerCommand);

CommandEnvelopeAuthRejectReason verifyCommandEnvelopeV1Auth(const CommandEnvelopeParseResult& envelope,
                                                            const Fw::ComBuffer& innerCommand,
                                                            const AuthorityConfig& config);

Fw::ComBuffer makeCommandEnvelopeV1(FwOpcodeType innerOpcode,
                                    U32 sourceId,
                                    U16 keySlot,
                                    const U8* keyBytes,
                                    FwSizeType keyLength,
                                    U32 sessionId,
                                    U32 sequenceNumber);

Fw::ComBuffer makeCommandEnvelopeV1(const Fw::ComBuffer& innerCommand,
                                    U32 sourceId,
                                    U16 keySlot,
                                    const U8* keyBytes,
                                    FwSizeType keyLength,
                                    U32 sessionId,
                                    U32 sequenceNumber);

}  // namespace OBC

#endif
