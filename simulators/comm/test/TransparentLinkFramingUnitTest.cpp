#include <iostream>
#include <string>

#include "simulators/comm/TransparentLinkFraming.hpp"

namespace {

bool check(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

bool testRoundTripPreservesEscapedPayloadAndMetadata() {
    bool ok = true;

    std::string payload;
    payload.push_back(static_cast<char>(0x00));
    payload.push_back(OBC::COMM::TRANSPARENT_FRAME_DELIMITER);
    payload.push_back(OBC::COMM::TRANSPARENT_FRAME_ESCAPE);
    payload += "ABC";

    const std::string encoded = OBC::COMM::encodeTransparentLinkFrame(payload, 0xA5U);
    ok = check(!encoded.empty() && encoded.back() == OBC::COMM::TRANSPARENT_FRAME_DELIMITER,
               "Expected encoded frame to end with the delimiter") &&
         ok;

    std::string decoded;
    OBC::COMM::TransparentFrameInfo info = {};
    const OBC::COMM::TransparentFrameStatus status =
        OBC::COMM::decodeTransparentLinkFrame(encoded.substr(0U, encoded.size() - 1U), decoded, &info);
    ok = check(status == OBC::COMM::TransparentFrameStatus::OK, "Expected transparent frame round-trip to decode") &&
         ok;
    ok = check(decoded == payload, "Expected decoded payload to match original binary payload") && ok;
    ok = check(info.version == OBC::COMM::TRANSPARENT_FRAME_VERSION,
               "Expected decoded transparent frame version metadata") &&
         ok;
    ok = check(info.flags == 0xA5U, "Expected decoded transparent frame flags metadata") && ok;

    return ok;
}

bool testDecodeSurfacesBadEscapeLengthAndCrcErrors() {
    bool ok = true;

    std::string badEscape;
    badEscape.push_back(OBC::COMM::TRANSPARENT_FRAME_ESCAPE);
    badEscape.push_back(static_cast<char>(0x00));
    std::string decoded;
    ok = check(OBC::COMM::decodeTransparentLinkFrame(badEscape, decoded, nullptr) ==
                   OBC::COMM::TransparentFrameStatus::BAD_ESCAPE,
               "Expected invalid escape sequence to report BAD_ESCAPE") &&
         ok;

    const std::string encoded = OBC::COMM::encodeTransparentLinkFrame("HELLO");
    std::string noDelimiter = encoded.substr(0U, encoded.size() - 1U);

    std::string lengthMismatch = noDelimiter;
    lengthMismatch[2] = 0x00;
    lengthMismatch[3] = 0x01;
    ok = check(OBC::COMM::decodeTransparentLinkFrame(lengthMismatch, decoded, nullptr) ==
                   OBC::COMM::TransparentFrameStatus::LENGTH_MISMATCH,
               "Expected mismatched payload length to report LENGTH_MISMATCH") &&
         ok;

    std::string crcMismatch = noDelimiter;
    crcMismatch[4] ^= 0x01;
    ok = check(OBC::COMM::decodeTransparentLinkFrame(crcMismatch, decoded, nullptr) ==
                   OBC::COMM::TransparentFrameStatus::CRC_MISMATCH,
               "Expected corrupted frame body to report CRC_MISMATCH") &&
         ok;

    return ok;
}

}  // namespace

int main() {
    bool ok = true;
    ok = testRoundTripPreservesEscapedPayloadAndMetadata() && ok;
    ok = testDecodeSurfacesBadEscapeLengthAndCrcErrors() && ok;
    return ok ? 0 : 1;
}
