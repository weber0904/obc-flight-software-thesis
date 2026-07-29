#include "OBC/Components/SequenceAdmissionController/SequenceFileInspector.hpp"

#include <fstream>
#include <vector>

#include "Fw/Com/ComPacket.hpp"
#include "Fw/Types/Serializable.hpp"

extern "C" {
#include "Utils/Hash/libcrc/lib_crc.h"
}

namespace OBC {

namespace {

constexpr U32 INITIAL_COMPUTED_VALUE = 0xFFFFFFFFU;
constexpr U32 HEADER_SIZE =
    sizeof(U32) + sizeof(U32) + sizeof(FwTimeBaseStoreType) + sizeof(FwTimeContextStoreType);

enum class SequenceRecordDescriptor : U8 {
    ABSOLUTE = 0,
    RELATIVE = 1,
    END_OF_SEQUENCE = 2,
};

void updateCrc(U32& crc, const U8* data, FwSizeType size) {
    for (FwSizeType i = 0; i < size; ++i) {
        crc = static_cast<U32>(update_crc_32(crc, static_cast<char>(data[i])));
    }
}

}  // namespace

SequenceInspectionResult SequenceFileInspector::inspect(const std::string& filePath, const Fw::Time& validTime) const {
    SequenceInspectionResult result;

    std::ifstream input(filePath, std::ios::binary);
    if (!input.is_open()) {
        result.error = SequenceInspectionError::FILE_NOT_FOUND;
        return result;
    }

    std::vector<U8> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad()) {
        result.error = SequenceInspectionError::FILE_READ_ERROR;
        return result;
    }

    if (bytes.size() < HEADER_SIZE + sizeof(U32)) {
        result.error = SequenceInspectionError::FILE_TOO_SMALL;
        return result;
    }

    Fw::ExternalSerializeBuffer headerBuffer(bytes.data(), static_cast<FwSizeType>(bytes.size()));
    if (headerBuffer.setBuffLen(static_cast<FwSizeType>(bytes.size())) != Fw::FW_SERIALIZE_OK) {
        result.error = SequenceInspectionError::FILE_TOO_LARGE;
        return result;
    }

    U32 fileSize = 0U;
    U32 numRecords = 0U;
    TimeBase timeBase = TimeBase::TB_DONT_CARE;
    FwTimeContextStoreType timeContext = FW_CONTEXT_DONT_CARE;

    if (headerBuffer.deserializeTo(fileSize) != Fw::FW_SERIALIZE_OK ||
        headerBuffer.deserializeTo(numRecords) != Fw::FW_SERIALIZE_OK ||
        headerBuffer.deserializeTo(timeBase) != Fw::FW_SERIALIZE_OK ||
        headerBuffer.deserializeTo(timeContext) != Fw::FW_SERIALIZE_OK) {
        result.error = SequenceInspectionError::INVALID_HEADER;
        return result;
    }

    const FwSizeType totalExpected = HEADER_SIZE + static_cast<FwSizeType>(fileSize);
    if (totalExpected != bytes.size()) {
        result.error = SequenceInspectionError::FILE_TOO_LARGE;
        result.detail = static_cast<U32>(bytes.size());
        return result;
    }

    if (numRecords == 0U) {
        result.error = SequenceInspectionError::NO_RECORDS;
        return result;
    }

    if (timeBase != TimeBase::TB_DONT_CARE && timeBase != validTime.getTimeBase()) {
        result.error = SequenceInspectionError::TIME_BASE_MISMATCH;
        result.detail = static_cast<U32>(timeBase);
        return result;
    }
    if (timeContext != FW_CONTEXT_DONT_CARE && timeContext != validTime.getContext()) {
        result.error = SequenceInspectionError::TIME_CONTEXT_MISMATCH;
        result.detail = static_cast<U32>(timeContext);
        return result;
    }

    U32 computedCrc = INITIAL_COMPUTED_VALUE;
    updateCrc(computedCrc, bytes.data(), HEADER_SIZE);

    const FwSizeType recordsSize = static_cast<FwSizeType>(fileSize) - sizeof(U32);
    updateCrc(computedCrc, bytes.data() + HEADER_SIZE, recordsSize);
    computedCrc = ~computedCrc;

    Fw::ExternalSerializeBuffer crcBuffer(bytes.data() + HEADER_SIZE + recordsSize, sizeof(U32));
    if (crcBuffer.setBuffLen(sizeof(U32)) != Fw::FW_SERIALIZE_OK) {
        result.error = SequenceInspectionError::INVALID_HEADER;
        return result;
    }

    U32 storedCrc = 0U;
    if (crcBuffer.deserializeTo(storedCrc) != Fw::FW_SERIALIZE_OK) {
        result.error = SequenceInspectionError::INVALID_HEADER;
        return result;
    }
    result.crc = storedCrc;
    if (storedCrc != computedCrc) {
        result.error = SequenceInspectionError::CRC_MISMATCH;
        result.detail = computedCrc;
        return result;
    }

    Fw::ExternalSerializeBuffer recordsBuffer(bytes.data() + HEADER_SIZE, recordsSize);
    if (recordsBuffer.setBuffLen(recordsSize) != Fw::FW_SERIALIZE_OK) {
        result.error = SequenceInspectionError::FILE_TOO_LARGE;
        return result;
    }

    for (U32 recordIndex = 0U; recordIndex < numRecords; ++recordIndex) {
        U8 descriptorValue = 0U;
        if (recordsBuffer.deserializeTo(descriptorValue) != Fw::FW_SERIALIZE_OK) {
            result.error = SequenceInspectionError::INVALID_RECORD;
            result.detail = recordIndex;
            return result;
        }
        if (descriptorValue > static_cast<U8>(SequenceRecordDescriptor::END_OF_SEQUENCE)) {
            result.error = SequenceInspectionError::INVALID_RECORD;
            result.detail = recordIndex;
            return result;
        }

        if (static_cast<SequenceRecordDescriptor>(descriptorValue) == SequenceRecordDescriptor::END_OF_SEQUENCE) {
            continue;
        }

        U32 seconds = 0U;
        U32 useconds = 0U;
        U32 recordSize = 0U;
        if (recordsBuffer.deserializeTo(seconds) != Fw::FW_SERIALIZE_OK ||
            recordsBuffer.deserializeTo(useconds) != Fw::FW_SERIALIZE_OK ||
            recordsBuffer.deserializeTo(recordSize) != Fw::FW_SERIALIZE_OK) {
            result.error = SequenceInspectionError::INVALID_RECORD;
            result.detail = recordIndex;
            return result;
        }

        if (recordSize > recordsBuffer.getDeserializeSizeLeft() ||
            recordSize + sizeof(FwPacketDescriptorType) > Fw::ComBuffer::SERIALIZED_SIZE) {
            result.error = SequenceInspectionError::INVALID_RECORD;
            result.detail = recordIndex;
            return result;
        }

        SequenceRecordInfo recordInfo;
        recordInfo.recordIndex = recordIndex;
        FwSizeType rawSize = static_cast<FwSizeType>(recordSize);
        if (recordInfo.command.setBuffLen(recordSize) != Fw::FW_SERIALIZE_OK ||
            recordsBuffer.deserializeTo(recordInfo.command.getBuffAddr(), rawSize, Fw::Serialization::OMIT_LENGTH) !=
                Fw::FW_SERIALIZE_OK) {
            result.error = SequenceInspectionError::INVALID_RECORD;
            result.detail = recordIndex;
            return result;
        }

        Fw::CmdPacket packet;
        if (packet.deserializeFrom(recordInfo.command) != Fw::FW_SERIALIZE_OK) {
            result.error = SequenceInspectionError::INVALID_COMMAND;
            result.detail = recordIndex;
            return result;
        }

        recordInfo.opcode = static_cast<U32>(packet.getOpCode());
        result.records.push_back(recordInfo);
    }

    if (recordsBuffer.getDeserializeSizeLeft() != 0U) {
        result.error = SequenceInspectionError::RECORD_MISMATCH;
        result.detail = static_cast<U32>(recordsBuffer.getDeserializeSizeLeft());
        return result;
    }

    result.valid = true;
    result.error = SequenceInspectionError::NONE;
    return result;
}

}  // namespace OBC
