#ifndef OBC_SEQUENCE_FILE_INSPECTOR_HPP
#define OBC_SEQUENCE_FILE_INSPECTOR_HPP

#include <string>
#include <vector>

#include "Fw/Cmd/CmdPacket.hpp"
#include "Fw/Com/ComBuffer.hpp"
#include "Fw/Time/Time.hpp"

namespace OBC {

enum class SequenceInspectionError : unsigned int {
    NONE = 0,
    FILE_NOT_FOUND = 1,
    FILE_READ_ERROR = 2,
    FILE_TOO_SMALL = 3,
    FILE_TOO_LARGE = 4,
    CRC_MISMATCH = 5,
    INVALID_HEADER = 6,
    TIME_BASE_MISMATCH = 7,
    TIME_CONTEXT_MISMATCH = 8,
    NO_RECORDS = 9,
    INVALID_RECORD = 10,
    RECORD_MISMATCH = 11,
    INVALID_COMMAND = 12,
    UNKNOWN_OPCODE = 13,
    AUTHORITY_DENIED = 14,
};

struct SequenceRecordInfo {
    U32 recordIndex = 0U;
    U32 opcode = 0U;
    Fw::ComBuffer command = {};
};

struct SequenceInspectionResult {
    bool valid = false;
    SequenceInspectionError error = SequenceInspectionError::NONE;
    U32 detail = 0U;
    U32 crc = 0U;
    std::vector<SequenceRecordInfo> records;
};

class SequenceFileInspector {
  public:
    SequenceInspectionResult inspect(const std::string& filePath, const Fw::Time& validTime) const;
};

}  // namespace OBC

#endif
