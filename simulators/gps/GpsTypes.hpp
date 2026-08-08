#ifndef OBC_SIMULATORS_GPS_GPSTYPES_HPP
#define OBC_SIMULATORS_GPS_GPSTYPES_HPP

#include <cstdint>

namespace OBC {
namespace GPS {

enum class SourceMode : std::uint8_t {
    FAKE = 0U,
    REPLAY = 1U,
    LIVE_UART = 2U,
};

enum class ParseStatus : std::uint8_t {
    OK = 0U,
    NO_FIX = 1U,
    UNSUPPORTED = 2U,
    INVALID_CHECKSUM = 3U,
    MALFORMED = 4U,
};

struct StateData {
    bool hasSample = false;
    bool fixValid = false;
    SourceMode sourceMode = SourceMode::FAKE;
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    float altitudeMeters = 0.0F;
    float speedMetersPerSecond = 0.0F;
    float courseDegrees = 0.0F;
    std::uint8_t satelliteCount = 0U;
    float hdop = 0.0F;
    std::uint32_t utcSecondsOfDay = 0U;
    std::uint32_t utcDateYmd = 0U;
    std::uint32_t acceptedSentenceCount = 0U;
    std::uint32_t rejectedSentenceCount = 0U;
};

struct SentenceUpdate {
    ParseStatus status = ParseStatus::UNSUPPORTED;
    bool updatesState = false;
    bool fixValid = false;
    bool hasLatitudeLongitude = false;
    bool hasAltitude = false;
    bool hasSpeed = false;
    bool hasCourse = false;
    bool hasSatelliteCount = false;
    bool hasHdop = false;
    bool hasUtcTime = false;
    bool hasUtcDate = false;
    double latitudeDeg = 0.0;
    double longitudeDeg = 0.0;
    float altitudeMeters = 0.0F;
    float speedMetersPerSecond = 0.0F;
    float courseDegrees = 0.0F;
    std::uint8_t satelliteCount = 0U;
    float hdop = 0.0F;
    std::uint32_t utcSecondsOfDay = 0U;
    std::uint32_t utcDateYmd = 0U;
};

}  // namespace GPS
}  // namespace OBC

#endif
