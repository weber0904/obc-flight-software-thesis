#ifndef OBC_OFFICIAL_SEQUENCE_OPCODES_HPP
#define OBC_OFFICIAL_SEQUENCE_OPCODES_HPP

#include "Fw/FPrimeBasicTypes.hpp"

namespace OBC {

constexpr FwOpcodeType OBC_FILE_INGRESS_AUTHORITY_BASE_ID = 0x10049000U;
constexpr FwOpcodeType OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID = 0x1004A000U;
constexpr FwOpcodeType OBC_CMD_SEQ_A_BASE_ID = 0x1004B000U;
constexpr FwOpcodeType OBC_CMD_SEQ_B_BASE_ID = 0x1004C000U;
constexpr FwOpcodeType OBC_SEQ_DISPATCHER_BASE_ID = 0x1004D000U;
constexpr FwOpcodeType OBC_SYSTEM_RESOURCES_BASE_ID = 0x10050000U;

constexpr FwOpcodeType OBC_SEQ_VALIDATE_OPCODE = OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID + 0x0U;
constexpr FwOpcodeType OBC_SEQ_RUN_OPCODE = OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID + 0x1U;
constexpr FwOpcodeType OBC_SEQ_PREPARE_MANUAL_OPCODE = OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID + 0x2U;
constexpr FwOpcodeType OBC_SEQ_START_OPCODE = OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID + 0x3U;
constexpr FwOpcodeType OBC_SEQ_STEP_OPCODE = OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID + 0x4U;
constexpr FwOpcodeType OBC_SEQ_CANCEL_OPCODE = OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID + 0x5U;
constexpr FwOpcodeType OBC_SEQ_LOG_STATUS_OPCODE = OBC_SEQUENCE_ADMISSION_CONTROLLER_BASE_ID + 0x6U;

constexpr FwOpcodeType OBC_CMD_SEQ_CS_RUN_OPCODE(FwOpcodeType baseId) { return baseId + 0x0U; }
constexpr FwOpcodeType OBC_CMD_SEQ_CS_VALIDATE_OPCODE(FwOpcodeType baseId) { return baseId + 0x1U; }
constexpr FwOpcodeType OBC_CMD_SEQ_CS_CANCEL_OPCODE(FwOpcodeType baseId) { return baseId + 0x2U; }
constexpr FwOpcodeType OBC_CMD_SEQ_CS_START_OPCODE(FwOpcodeType baseId) { return baseId + 0x3U; }
constexpr FwOpcodeType OBC_CMD_SEQ_CS_STEP_OPCODE(FwOpcodeType baseId) { return baseId + 0x4U; }
constexpr FwOpcodeType OBC_CMD_SEQ_CS_AUTO_OPCODE(FwOpcodeType baseId) { return baseId + 0x5U; }
constexpr FwOpcodeType OBC_CMD_SEQ_CS_MANUAL_OPCODE(FwOpcodeType baseId) { return baseId + 0x6U; }
constexpr FwOpcodeType OBC_CMD_SEQ_CS_JOIN_WAIT_OPCODE(FwOpcodeType baseId) { return baseId + 0x7U; }

constexpr FwOpcodeType OBC_SEQ_DISPATCHER_RUN_OPCODE = OBC_SEQ_DISPATCHER_BASE_ID + 0x0U;
constexpr FwOpcodeType OBC_SEQ_DISPATCHER_LOG_STATUS_OPCODE = OBC_SEQ_DISPATCHER_BASE_ID + 0x1U;

constexpr FwOpcodeType OBC_SYSTEM_RESOURCES_ENABLE_OPCODE = OBC_SYSTEM_RESOURCES_BASE_ID + 0x0U;

}  // namespace OBC

#endif
