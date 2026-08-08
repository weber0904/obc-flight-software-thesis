#include "simulators/gps/NmeaParser.hpp"

#include <cctype>
#include <cstdlib>
#include <vector>

namespace OBC {
namespace GPS {

namespace {

constexpr std::size_t MAX_NMEA_SENTENCE_LENGTH = 96U;
constexpr std::size_t MAX_NMEA_FIELD_COUNT = 24U;

bool splitFields(const std::string& payload, std::vector<std::string>& fields) {
    fields.clear();
    fields.reserve(MAX_NMEA_FIELD_COUNT);
    std::string current;
    for (char ch : payload) {
        if (ch == ',') {
            if (fields.size() >= MAX_NMEA_FIELD_COUNT) {
                fields.clear();
                return false;
            }
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (fields.size() >= MAX_NMEA_FIELD_COUNT) {
        fields.clear();
        return false;
    }
    fields.push_back(current);
    return !fields.empty();
}

bool parseUnsigned(const std::string& text, std::uint32_t& value) {
    if (text.empty()) {
        return false;
    }
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(text.c_str(), &end, 10);
    if (end == nullptr || *end != '\0') {
        return false;
    }
    value = static_cast<std::uint32_t>(parsed);
    return true;
}

bool parseFloat(const std::string& text, float& value) {
    if (text.empty()) {
        return false;
    }
    char* end = nullptr;
    value = std::strtof(text.c_str(), &end);
    return end != nullptr && *end == '\0';
}

bool parseUtcTime(const std::string& text, std::uint32_t& secondsOfDay) {
    if (text.size() < 6U) {
        return false;
    }

    const std::uint32_t hours = static_cast<std::uint32_t>((text[0] - '0') * 10 + (text[1] - '0'));
    const std::uint32_t minutes = static_cast<std::uint32_t>((text[2] - '0') * 10 + (text[3] - '0'));
    const std::uint32_t seconds = static_cast<std::uint32_t>((text[4] - '0') * 10 + (text[5] - '0'));
    if (hours > 23U || minutes > 59U || seconds > 59U) {
        return false;
    }
    secondsOfDay = (hours * 3600U) + (minutes * 60U) + seconds;
    return true;
}

bool parseUtcDate(const std::string& text, std::uint32_t& ymd) {
    if (text.size() != 6U) {
        return false;
    }

    const std::uint32_t day = static_cast<std::uint32_t>((text[0] - '0') * 10 + (text[1] - '0'));
    const std::uint32_t month = static_cast<std::uint32_t>((text[2] - '0') * 10 + (text[3] - '0'));
    const std::uint32_t year2 = static_cast<std::uint32_t>((text[4] - '0') * 10 + (text[5] - '0'));
    if (day == 0U || day > 31U || month == 0U || month > 12U) {
        return false;
    }
    const std::uint32_t year = year2 >= 80U ? (1900U + year2) : (2000U + year2);
    ymd = (year * 10000U) + (month * 100U) + day;
    return true;
}

bool parseCoordinate(const std::string& text, char hemisphere, bool isLatitude, double& degrees) {
    if (text.empty()) {
        return false;
    }

    std::size_t point = text.find('.');
    if (point == std::string::npos || point < (isLatitude ? 4U : 5U)) {
        return false;
    }

    const std::size_t degreeDigits = isLatitude ? 2U : 3U;
    if (text.size() < degreeDigits + 2U) {
        return false;
    }

    const std::string degreeText = text.substr(0U, degreeDigits);
    const std::string minuteText = text.substr(degreeDigits);

    char* end = nullptr;
    const double deg = std::strtod(degreeText.c_str(), &end);
    if (end == nullptr || *end != '\0') {
        return false;
    }
    const double minutes = std::strtod(minuteText.c_str(), &end);
    if (end == nullptr || *end != '\0') {
        return false;
    }

    degrees = deg + (minutes / 60.0);
    if (hemisphere == 'S' || hemisphere == 'W') {
        degrees = -degrees;
    } else if (!(hemisphere == 'N' || hemisphere == 'E')) {
        return false;
    }

    return true;
}

bool verifyChecksum(const std::string& sentence, std::string& payload) {
    payload.clear();
    if (sentence.empty()) {
        return false;
    }

    std::size_t start = sentence[0] == '$' ? 1U : 0U;
    std::size_t end = sentence.find('*');
    if (end == std::string::npos || end + 3U > sentence.size()) {
        return false;
    }

    payload = sentence.substr(start, end - start);
    std::uint8_t checksum = 0U;
    for (char ch : payload) {
        checksum ^= static_cast<std::uint8_t>(ch);
    }

    auto decodeHex = [](char ch, std::uint8_t& value) -> bool {
        if (ch >= '0' && ch <= '9') {
            value = static_cast<std::uint8_t>(ch - '0');
            return true;
        }
        if (ch >= 'A' && ch <= 'F') {
            value = static_cast<std::uint8_t>(10 + (ch - 'A'));
            return true;
        }
        if (ch >= 'a' && ch <= 'f') {
            value = static_cast<std::uint8_t>(10 + (ch - 'a'));
            return true;
        }
        return false;
    };

    std::uint8_t hi = 0U;
    std::uint8_t lo = 0U;
    if (!decodeHex(sentence[end + 1U], hi) || !decodeHex(sentence[end + 2U], lo)) {
        return false;
    }
    return checksum == static_cast<std::uint8_t>((hi << 4U) | lo);
}

ParseStatus parseGga(const std::vector<std::string>& fields, SentenceUpdate& update) {
    if (fields.size() < 10U) {
        return ParseStatus::MALFORMED;
    }

    update = {};
    update.updatesState = true;

    if (!fields[1].empty()) {
        update.hasUtcTime = parseUtcTime(fields[1], update.utcSecondsOfDay);
    }
    if (!fields[7].empty()) {
        std::uint32_t satellites = 0U;
        if (parseUnsigned(fields[7], satellites) && satellites <= 255U) {
            update.hasSatelliteCount = true;
            update.satelliteCount = static_cast<std::uint8_t>(satellites);
        }
    }
    if (!fields[8].empty()) {
        update.hasHdop = parseFloat(fields[8], update.hdop);
    }

    std::uint32_t fixQuality = 0U;
    if (!parseUnsigned(fields[6], fixQuality)) {
        return ParseStatus::MALFORMED;
    }
    if (fixQuality == 0U) {
        update.status = ParseStatus::NO_FIX;
        update.fixValid = false;
        return update.status;
    }

    if (!parseCoordinate(fields[2], fields[3].empty() ? '\0' : fields[3][0], true, update.latitudeDeg) ||
        !parseCoordinate(fields[4], fields[5].empty() ? '\0' : fields[5][0], false, update.longitudeDeg)) {
        return ParseStatus::MALFORMED;
    }
    update.hasLatitudeLongitude = true;

    if (!fields[9].empty()) {
        update.hasAltitude = parseFloat(fields[9], update.altitudeMeters);
    }

    update.status = ParseStatus::OK;
    update.fixValid = true;
    return update.status;
}

ParseStatus parseRmc(const std::vector<std::string>& fields, SentenceUpdate& update) {
    if (fields.size() < 10U) {
        return ParseStatus::MALFORMED;
    }

    update = {};
    update.updatesState = true;

    if (!fields[1].empty()) {
        update.hasUtcTime = parseUtcTime(fields[1], update.utcSecondsOfDay);
    }
    if (!fields[9].empty()) {
        update.hasUtcDate = parseUtcDate(fields[9], update.utcDateYmd);
    }

    const std::string& statusField = fields[2];
    if (statusField.size() != 1U) {
        return ParseStatus::MALFORMED;
    }
    if (statusField[0] == 'V') {
        update.status = ParseStatus::NO_FIX;
        update.fixValid = false;
        return update.status;
    }
    if (statusField[0] != 'A') {
        return ParseStatus::MALFORMED;
    }

    if (!parseCoordinate(fields[3], fields[4].empty() ? '\0' : fields[4][0], true, update.latitudeDeg) ||
        !parseCoordinate(fields[5], fields[6].empty() ? '\0' : fields[6][0], false, update.longitudeDeg)) {
        return ParseStatus::MALFORMED;
    }
    update.hasLatitudeLongitude = true;

    if (!fields[7].empty()) {
        float speedKnots = 0.0F;
        if (!parseFloat(fields[7], speedKnots)) {
            return ParseStatus::MALFORMED;
        }
        update.hasSpeed = true;
        update.speedMetersPerSecond = speedKnots * 0.514444F;
    }

    if (!fields[8].empty()) {
        update.hasCourse = parseFloat(fields[8], update.courseDegrees);
    }

    update.status = ParseStatus::OK;
    update.fixValid = true;
    return update.status;
}

}  // namespace

ParseStatus parseNmeaSentence(const std::string& sentence, SentenceUpdate& update) {
    update = {};

    std::string trimmed = sentence;
    while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == '\n' || std::isspace(trimmed.back()))) {
        trimmed.pop_back();
    }
    if (trimmed.empty()) {
        return ParseStatus::MALFORMED;
    }
    if (trimmed.size() > MAX_NMEA_SENTENCE_LENGTH) {
        return ParseStatus::MALFORMED;
    }

    std::string payload;
    if (!verifyChecksum(trimmed, payload)) {
        return ParseStatus::INVALID_CHECKSUM;
    }

    std::vector<std::string> fields;
    if (!splitFields(payload, fields) || fields.empty()) {
        return ParseStatus::MALFORMED;
    }

    const std::string& sentenceId = fields[0];
    if (sentenceId.size() < 5U) {
        return ParseStatus::MALFORMED;
    }

    const std::string kind = sentenceId.substr(sentenceId.size() - 3U);
    if (kind == "GGA") {
        return parseGga(fields, update);
    }
    if (kind == "RMC") {
        return parseRmc(fields, update);
    }

    return ParseStatus::UNSUPPORTED;
}

}  // namespace GPS
}  // namespace OBC
