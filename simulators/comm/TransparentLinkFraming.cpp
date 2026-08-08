#include "simulators/comm/TransparentLinkFraming.hpp"

#include <cstddef>

namespace OBC {
namespace COMM {

namespace {

std::uint32_t crc32(const std::string& payload) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (unsigned char byte : payload) {
        crc ^= static_cast<std::uint32_t>(byte);
        for (unsigned int bit = 0U; bit < 8U; ++bit) {
            const bool lsb = (crc & 1U) != 0U;
            crc >>= 1U;
            if (lsb) {
                crc ^= 0xEDB88320U;
            }
        }
    }
    return ~crc;
}

void appendEscapedByte(std::string& output, unsigned char value) {
    if (value == static_cast<unsigned char>(TRANSPARENT_FRAME_DELIMITER)) {
        output.push_back(TRANSPARENT_FRAME_ESCAPE);
        output.push_back(static_cast<char>(0x5E));
        return;
    }
    if (value == static_cast<unsigned char>(TRANSPARENT_FRAME_ESCAPE)) {
        output.push_back(TRANSPARENT_FRAME_ESCAPE);
        output.push_back(static_cast<char>(0x5D));
        return;
    }
    output.push_back(static_cast<char>(value));
}

bool unescapePayload(const std::string& encodedFrame, std::string& decoded) {
    decoded.clear();
    bool escaping = false;
    for (unsigned char value : encodedFrame) {
        if (!escaping) {
            if (value == static_cast<unsigned char>(TRANSPARENT_FRAME_ESCAPE)) {
                escaping = true;
            } else {
                decoded.push_back(static_cast<char>(value));
            }
            continue;
        }

        escaping = false;
        if (value == 0x5E) {
            decoded.push_back(TRANSPARENT_FRAME_DELIMITER);
            continue;
        }
        if (value == 0x5D) {
            decoded.push_back(TRANSPARENT_FRAME_ESCAPE);
            continue;
        }
        return false;
    }
    return !escaping;
}

std::uint16_t readBe16(const std::string& payload, std::size_t offset) {
    return static_cast<std::uint16_t>((static_cast<unsigned char>(payload[offset]) << 8U) |
                                      static_cast<unsigned char>(payload[offset + 1U]));
}

std::uint32_t readBe32(const std::string& payload, std::size_t offset) {
    return (static_cast<std::uint32_t>(static_cast<unsigned char>(payload[offset])) << 24U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(payload[offset + 1U])) << 16U) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(payload[offset + 2U])) << 8U) |
           static_cast<std::uint32_t>(static_cast<unsigned char>(payload[offset + 3U]));
}

void appendBe16(std::string& payload, std::uint16_t value) {
    payload.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    payload.push_back(static_cast<char>(value & 0xFFU));
}

void appendBe32(std::string& payload, std::uint32_t value) {
    payload.push_back(static_cast<char>((value >> 24U) & 0xFFU));
    payload.push_back(static_cast<char>((value >> 16U) & 0xFFU));
    payload.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    payload.push_back(static_cast<char>(value & 0xFFU));
}

}  // namespace

std::string encodeTransparentLinkFrame(const std::string& payload, std::uint8_t flags) {
    std::string body;
    body.reserve(2U + 2U + payload.size() + 4U);
    body.push_back(static_cast<char>(TRANSPARENT_FRAME_VERSION));
    body.push_back(static_cast<char>(flags));
    appendBe16(body, static_cast<std::uint16_t>(payload.size()));
    body += payload;
    appendBe32(body, crc32(body));

    std::string encoded;
    encoded.reserve(body.size() + 1U);
    for (unsigned char value : body) {
        appendEscapedByte(encoded, value);
    }
    encoded.push_back(TRANSPARENT_FRAME_DELIMITER);
    return encoded;
}

TransparentFrameStatus decodeTransparentLinkFrame(const std::string& encodedFrame,
                                                  std::string& payload,
                                                  TransparentFrameInfo* info) {
    payload.clear();

    std::string body;
    if (!unescapePayload(encodedFrame, body)) {
        return TransparentFrameStatus::BAD_ESCAPE;
    }
    if (body.size() < 8U) {
        return TransparentFrameStatus::TOO_SHORT;
    }

    const std::uint8_t version = static_cast<std::uint8_t>(body[0]);
    const std::uint8_t flags = static_cast<std::uint8_t>(body[1]);
    if (version != TRANSPARENT_FRAME_VERSION) {
        return TransparentFrameStatus::UNSUPPORTED_VERSION;
    }

    const std::uint16_t payloadLength = readBe16(body, 2U);
    const std::size_t expectedSize = 2U + 2U + static_cast<std::size_t>(payloadLength) + 4U;
    if (body.size() != expectedSize) {
        return TransparentFrameStatus::LENGTH_MISMATCH;
    }

    const std::uint32_t expectedCrc = readBe32(body, body.size() - 4U);
    const std::string crcInput = body.substr(0U, body.size() - 4U);
    if (crc32(crcInput) != expectedCrc) {
        return TransparentFrameStatus::CRC_MISMATCH;
    }

    payload.assign(body.begin() + 4, body.end() - 4);
    if (info != nullptr) {
        info->version = version;
        info->flags = flags;
    }
    return TransparentFrameStatus::OK;
}

const char* transparentFrameStatusName(TransparentFrameStatus status) {
    switch (status) {
        case TransparentFrameStatus::OK:
            return "ok";
        case TransparentFrameStatus::BAD_ESCAPE:
            return "bad-escape";
        case TransparentFrameStatus::TOO_SHORT:
            return "too-short";
        case TransparentFrameStatus::UNSUPPORTED_VERSION:
            return "unsupported-version";
        case TransparentFrameStatus::LENGTH_MISMATCH:
            return "length-mismatch";
        case TransparentFrameStatus::CRC_MISMATCH:
            return "crc-mismatch";
        default:
            return "unknown";
    }
}

}  // namespace COMM
}  // namespace OBC
