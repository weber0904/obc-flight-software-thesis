#ifndef OBC_COMM_TransparentLinkFraming_HPP
#define OBC_COMM_TransparentLinkFraming_HPP

#include <cstdint>
#include <string>

namespace OBC {
namespace COMM {

enum class TransparentFrameStatus {
    OK,
    BAD_ESCAPE,
    TOO_SHORT,
    UNSUPPORTED_VERSION,
    LENGTH_MISMATCH,
    CRC_MISMATCH
};

struct TransparentFrameInfo {
    std::uint8_t version;
    std::uint8_t flags;
};

constexpr char TRANSPARENT_FRAME_DELIMITER = static_cast<char>(0x7E);
constexpr char TRANSPARENT_FRAME_ESCAPE = static_cast<char>(0x7D);
constexpr std::uint8_t TRANSPARENT_FRAME_VERSION = 1U;

std::string encodeTransparentLinkFrame(const std::string& payload, std::uint8_t flags = 0U);

TransparentFrameStatus decodeTransparentLinkFrame(const std::string& encodedFrame,
                                                  std::string& payload,
                                                  TransparentFrameInfo* info = nullptr);

const char* transparentFrameStatusName(TransparentFrameStatus status);

}  // namespace COMM
}  // namespace OBC

#endif
