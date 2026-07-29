#ifndef OBC_COMPONENTS_TEST_OFFICIALSEQUENCETESTSUPPORT_HPP
#define OBC_COMPONENTS_TEST_OFFICIALSEQUENCETESTSUPPORT_HPP

#include <array>
#include <vector>

#include "Fw/Com/ComBuffer.hpp"
#include "Fw/Time/Time.hpp"
#include "Fw/Types/Serializable.hpp"
#include "OBC/Components/test/TestSupport.hpp"

extern "C" {
#include "Utils/Hash/libcrc/lib_crc.h"
}

namespace OBC {
namespace TestSupport {

struct SequenceRecordSpec {
    U8 descriptor = 0U;
    U32 seconds = 0U;
    U32 useconds = 0U;
    std::vector<U8> commandBytes;
};

inline void updateSequenceCrc(U32& crc, const U8* data, FwSizeType size) {
    for (FwSizeType i = 0; i < size; ++i) {
        crc = static_cast<U32>(update_crc_32(crc, static_cast<char>(data[i])));
    }
}

template <typename T>
inline bool appendSerializedValue(std::vector<U8>& bytes, const T& value) {
    std::array<U8, 64> scratch = {};
    Fw::ExternalSerializeBuffer buffer(scratch.data(), scratch.size());
    if (buffer.serializeFrom(value) != Fw::FW_SERIALIZE_OK) {
        return false;
    }
    bytes.insert(bytes.end(), buffer.getBuffAddr(), buffer.getBuffAddr() + buffer.getSize());
    return true;
}

inline std::vector<U8> makeCommandBytes(FwOpcodeType opcode, const std::vector<U8>& argBytes = {}) {
    Fw::ComBuffer packet;
    if (packet.serializeFrom(static_cast<FwPacketDescriptorType>(Fw::ComPacketType::FW_PACKET_COMMAND)) !=
            Fw::FW_SERIALIZE_OK ||
        packet.serializeFrom(opcode) != Fw::FW_SERIALIZE_OK) {
        return {};
    }
    if (!argBytes.empty() &&
        packet.serializeFrom(argBytes.data(), static_cast<FwSizeType>(argBytes.size()), Fw::Serialization::OMIT_LENGTH) !=
            Fw::FW_SERIALIZE_OK) {
        return {};
    }
    return std::vector<U8>(packet.getBuffAddr(), packet.getBuffAddr() + packet.getSize());
}

inline bool writeOfficialSequenceFile(const std::string& path,
                                      const std::vector<SequenceRecordSpec>& records,
                                      TimeBase timeBase = TimeBase::TB_DONT_CARE,
                                      FwTimeContextStoreType timeContext = FW_CONTEXT_DONT_CARE) {
    std::vector<U8> recordBytes;
    for (const SequenceRecordSpec& record : records) {
        if (!appendSerializedValue(recordBytes, record.descriptor)) {
            return false;
        }
        if (record.descriptor == 2U) {
            continue;
        }
        if (!appendSerializedValue(recordBytes, record.seconds) || !appendSerializedValue(recordBytes, record.useconds) ||
            !appendSerializedValue(recordBytes, static_cast<U32>(record.commandBytes.size()))) {
            return false;
        }
        recordBytes.insert(recordBytes.end(), record.commandBytes.begin(), record.commandBytes.end());
    }

    std::vector<U8> fileBytes;
    const U32 fileSize = static_cast<U32>(recordBytes.size() + sizeof(U32));
    if (!appendSerializedValue(fileBytes, fileSize) ||
        !appendSerializedValue(fileBytes, static_cast<U32>(records.size())) ||
        !appendSerializedValue(fileBytes, timeBase) || !appendSerializedValue(fileBytes, timeContext)) {
        return false;
    }
    const FwSizeType headerSize = static_cast<FwSizeType>(fileBytes.size());
    fileBytes.insert(fileBytes.end(), recordBytes.begin(), recordBytes.end());

    U32 crc = 0xFFFFFFFFU;
    updateSequenceCrc(crc, fileBytes.data(), headerSize);
    if (!recordBytes.empty()) {
        updateSequenceCrc(crc, recordBytes.data(), static_cast<FwSizeType>(recordBytes.size()));
    }
    crc = ~crc;
    if (!appendSerializedValue(fileBytes, crc)) {
        return false;
    }

    return writeBinaryFile(path, fileBytes);
}

}  // namespace TestSupport
}  // namespace OBC

#endif
